#include "driverlib.h"
#include "device.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//---------------------------------- DEFINITIONS ----------------------------------

// RS422 Pin Definitions
#define RS422_TE_EN_GPIO       37
#define RS422_TX_GPIO       38
#define RS422_RX_GPIO       39
#define RS422_TX_EN_GPIO   40
#define RS422_RX_ENn_GPIO   41

// LED Definitions
#define LED12 67
#define LED10 68
#define LED11 69
#define LED1  70
#define LED2  71
#define LED3  76

// Temperature Sensor Address and Register Addresses Defined
#define TEMP_SENSOR_ADDR  0x48
#define TEMP_REG_ADDR     0x00

#define HUMIDITY_ADDR     0x27
#define PRESSURE_ADDR     0x77

#define CMD_RESET           0x1E
#define CMD_ADC_READ        0x00
#define CMD_CONVERT_D1_OSR  0x48
#define CMD_CONVERT_D2_OSR  0x58
#define CMD_PROM_READ_BASE  0xA0

// PROM calibration coefficients
static uint16_t C[7] = {0};
static int32_t dT_global = 0;
static bool ms5607_initialized = false;

//---------------------------------- FUNCTION DECLARATIONS ----------------------------------
void delay_ms(uint32_t ms);
void init_card(void);
void config_clk(void);
void config_pins(void);
void init_I2C_perip(void);
void init_uart_rs422(void);

void turn_on_led(uint32_t led_id);
void turn_off_led(uint32_t led_id);

uint16_t read_temp_tmp100(void);
uint16_t read_temp_hih8131(void);
uint16_t read_humidity_hih8131(void);
float read_pressure_ms5607(void);
float read_temp_ms5607(void);

void ms5607_init_once(void);
static void ms5607_send_command(uint8_t cmd);
static uint32_t ms5607_read_adc(void);
static uint16_t ms5607_read_prom(uint8_t index);

void send_msg(uint16_t header, uint16_t msg);
void send_ok(void);
void send_fail(void);
void send_float(float value);

void cmd_decoder(uint16_t cmd);

//---------------------------------- FUNCTION DEFINITIONS ----------------------------------

void delay_ms(uint32_t ms) {
    DEVICE_DELAY_US(ms * 1000);
}

void init_card(void) {
    config_clk();
    config_pins();
}

void config_clk(void) {
    SysCtl_setClock(DEVICE_SETCLOCK_CFG);
}

void config_pins(void) {
    uint32_t led_pins[] = {LED12, LED10, LED11, LED1, LED2, LED3};
    uint32_t pin_mux[]  = {GPIO_67_GPIO67, GPIO_68_GPIO68, GPIO_69_GPIO69,
                           GPIO_70_GPIO70, GPIO_71_GPIO71, GPIO_76_GPIO76};
    for (int i = 0; i < 6; i++) {
        GPIO_setDirectionMode(led_pins[i], GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(led_pins[i], GPIO_PIN_TYPE_STD);
        GPIO_setPinConfig(pin_mux[i]);
        GPIO_writePin(led_pins[i], 1);
    }

    // RS422 control GPIOs
    GPIO_setDirectionMode(RS422_TE_EN_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(RS422_TX_EN_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(RS422_RX_ENn_GPIO, GPIO_DIR_MODE_OUT);

    GPIO_setPadConfig(RS422_TE_EN_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(RS422_TX_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(RS422_RX_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(RS422_TX_EN_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(RS422_RX_ENn_GPIO, GPIO_PIN_TYPE_STD);

    // Configure TX/RX pins for SCIC
    GPIO_setPinConfig(GPIO_37_GPIO37);
    GPIO_setPinConfig(GPIO_38_SCIC_TX);  // TX pin
    GPIO_setPinConfig(GPIO_39_SCIC_RX);  // RX pin
    GPIO_setPinConfig(GPIO_40_GPIO40);
    GPIO_setPinConfig(GPIO_41_GPIO41);

    GPIO_writePin(RS422_TE_EN_GPIO, 1);          // Enable RS422 (if applicable)
    GPIO_writePin(RS422_TX_EN_GPIO, 1);      // Disable TX initially (active low)
    GPIO_writePin(RS422_RX_ENn_GPIO, 0);      // Enable RX (active low)

    // I2C_EL (I2C-A) pinleri
    GPIO_setPinConfig(GPIO_33_I2CA_SCL);
    GPIO_setPadConfig(33, GPIO_PIN_TYPE_PULLUP);          // Open-drain + pull-up
    GPIO_setQualificationMode(33, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_32_I2CA_SDA);
    GPIO_setPadConfig(32, GPIO_PIN_TYPE_PULLUP);          // Open-drain + pull-up
    GPIO_setQualificationMode(32, GPIO_QUAL_ASYNC);


}

void init_I2C_perip(void)
{
    I2C_initMaster(I2CA_BASE, DEVICE_SYSCLK_FREQ, 400000, I2C_DUTYCYCLE_50);
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_enableModule(I2CA_BASE);
    I2C_disableLoopback(I2CA_BASE);
}

void reset_i2c_bus(void) {
    I2C_disableModule(I2CA_BASE);
    DEVICE_DELAY_US(100);
    I2C_enableModule(I2CA_BASE);
}

void cmd_decoder(uint16_t cmd)
{
    uint16_t tag = cmd >> 8; // shift right to get the first byte at the beginning
    if(tag == 0xFA)
    {
        cmd = cmd & 0x003F; // Masking process in order to get 6 bits in the middle part which corresponds the leds
        uint32_t led_pins[] = {LED3, LED2, LED1, LED11, LED10, LED12};
        uint16_t lsb;
        for (int i = 0;  i < 6; i++) {
            lsb = cmd & 0x0001; // fetch the lsb
            GPIO_writePin(led_pins[i], lsb); // turn on / off the led at the corresponding position according to the bit value at there.
            send_ok();
            cmd = cmd >> 1; // shift cmd to right by 1 bit to get the next bit
        }
    }
    else if(tag == 0xFB)
    {
        send_msg(0xBF00,read_temp_tmp100()); // Send 2 byte raw temperature value of tmp100 through RS422 UART port with header 0xBF00
    }
    else if(tag == 0xFC)
    {
        cmd = cmd & 0x00FF;
        if(cmd == 0x00AA)
        {
            send_msg(0xCFAA, read_temp_hih8131());  // Send 2 byte raw temperature value of hih8131 through RS422 UART port with header 0xCFAA
        }
        else if(cmd == 0x00AB)
        {
            send_msg(0xCFBA, read_humidity_hih8131());  // Send 2 byte raw humidity value of hih8131 through RS422 UART port with header 0xCFBA
        }
        else
        {
            send_fail(); // Send a fail message in case of no tag is found
        }
    }
    else if(tag == 0xFD)
    {
        cmd = cmd & 0x00FF;
        if(cmd == 0x00AA)
        {
            send_float(read_temp_ms5607()); // Read temp from MS5607
        }
        else if(cmd == 0x00AB)
        {
            send_float(read_pressure_ms5607()); // Read pressure from MS5607
        }
        else
        {
            send_fail(); // Send a fail message in case of no tag is found
        }
    }
    else {
        send_fail(); // Send a fail message in case of no tag is found

    }
}

void init_uart_rs422(void) {
    SCI_performSoftwareReset(SCIC_BASE);
    SCI_setConfig(SCIC_BASE, DEVICE_LSPCLK_FREQ, 115200, (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIC_BASE);
    SCI_enableModule(SCIC_BASE);
    SCI_enableFIFO(SCIC_BASE);
}

void rs422_enable_tx(void) {
    GPIO_writePin(RS422_TX_EN_GPIO, 1);  // Enable TX (active low)
    GPIO_writePin(RS422_RX_ENn_GPIO, 1);  // Disable RX
    delay_ms(10);  // Short delay for line stabilization
}

void rs422_disable_tx(void) {
    delay_ms(10);  // Ensure all chars transmitted
    GPIO_writePin(RS422_TX_EN_GPIO, 0);  // Disable TX
    GPIO_writePin(RS422_RX_ENn_GPIO, 0);  // Enable RX
}

void send_float(float value) {
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * 100);  // two digits after point

    if (frac_part < 0) frac_part = -frac_part;  // handle negatives

    char buffer[20];
    sprintf(buffer, "%d.%02d\n", int_part, frac_part);

    rs422_enable_tx();
    for (int i = 0; buffer[i]; i++)
        SCI_writeCharBlockingNonFIFO(SCIC_BASE, buffer[i]);
    rs422_disable_tx();
}


void send_ok(void) {
    rs422_enable_tx();
    delay_ms(2);
    SCI_writeCharBlockingFIFO(SCIC_BASE, 'O');
    SCI_writeCharBlockingFIFO(SCIC_BASE, 'K');
    delay_ms(2);
    rs422_disable_tx();
}

void send_msg(uint16_t header, uint16_t msg)
{
    uint8_t header_high = (header >> 8) & 0xFF;
    uint8_t header_low  = header & 0xFF;

    uint8_t msg_high = (msg >> 8) & 0xFF;
    uint8_t msg_low  = msg & 0xFF;

    rs422_enable_tx();
    delay_ms(2);

    SCI_writeCharBlockingFIFO(SCIC_BASE, header_high); // Header MSB
    SCI_writeCharBlockingFIFO(SCIC_BASE, header_low);  // Header LSB
    SCI_writeCharBlockingFIFO(SCIC_BASE, msg_high);    // Msg MSB
    SCI_writeCharBlockingFIFO(SCIC_BASE, msg_low);     // Msg LSB

    delay_ms(2);
    rs422_disable_tx();
}



void send_fail(void) {
    rs422_enable_tx();
    delay_ms(2);
    SCI_writeCharBlockingFIFO(SCIC_BASE, 'F');
    SCI_writeCharBlockingFIFO(SCIC_BASE, 'A');
    delay_ms(2);
    rs422_disable_tx();
}

void turn_on_led(uint32_t led_id) {
    GPIO_writePin(led_id, 0);
}

void turn_off_led(uint32_t led_id) {
    GPIO_writePin(led_id, 1);
}

// TMP100 Temp Sensor
uint16_t read_temp_tmp100(void)
{
    uint16_t msb, lsb;
    uint32_t timeout;

    // TMP100 adresi (7-bit)
    I2C_setSlaveAddress(I2CA_BASE, TEMP_SENSOR_ADDR);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_putData(I2CA_BASE, TEMP_REG_ADDR);  // Temperature register address

    I2C_sendStartCondition(I2CA_BASE);

    timeout = 1000;
    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_BYTE_SENT)) {
        if (timeout-- == 0) {
            reset_i2c_bus();
            return 0xFFFF;
        }
    }

    // Repeated Start for Read
    I2C_setDataCount(I2CA_BASE, 2);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE);
    I2C_sendStartCondition(I2CA_BASE);

    // 2 byte okuma
    timeout = 1000;
    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY)) {
        if (timeout-- == 0) {
            reset_i2c_bus();
            return 0xFFFF;
        }
    }
    msb = I2C_getData(I2CA_BASE);

    timeout = 1000;
    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY)) {
        if (timeout-- == 0) {
            reset_i2c_bus();
            return 0xFFFF;
        }
    }
    lsb = I2C_getData(I2CA_BASE);

    I2C_sendStopCondition(I2CA_BASE);

    return ((uint16_t)msb << 8) | lsb;
}

// HIH8131 Sensor
// Read 2 Byte raw temperature value from HIH8131
uint16_t read_temp_hih8131(void) {
    I2C_setDataCount(I2CA_BASE, 4);
    I2C_setTargetAddress(I2CA_BASE, HUMIDITY_ADDR);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE);
    I2C_sendStartCondition(I2CA_BASE);

    delay_ms(15); // 15ms delay

    uint16_t data[4] = {0};
    for (int i = 0; i < 4; i++) {
        while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY));
        data[i] = I2C_getData(I2CA_BASE);
    }

    I2C_sendStopCondition(I2CA_BASE);
    while (I2C_isBusBusy(I2CA_BASE));

    uint16_t temp_raw = ((data[2] << 6) + (data[3] >> 2)); // MSB (8 bits) of Temperature at data[2] and LSB (6 bits) of Temperature at data[3]
    return temp_raw;
}

// Read 2 Byte raw humidity value from HIH8131
uint16_t read_humidity_hih8131(void) {
    I2C_setDataCount(I2CA_BASE, 4);
    I2C_setTargetAddress(I2CA_BASE, HUMIDITY_ADDR);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE);
    I2C_sendStartCondition(I2CA_BASE);


    uint16_t data[4] = {0};
    for (int i = 0; i < 4; i++) {
        while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY));
        delay_ms(15); // 15ms delay
        data[i] = I2C_getData(I2CA_BASE);
    }

    I2C_sendStopCondition(I2CA_BASE);
    while (I2C_isBusBusy(I2CA_BASE));

    uint16_t raw_humidity = (data[0] << 8) + data[1]; // MSB (6 bits) of Humidity at data[0] and LSB (8 bits) of Humidity at data[1]
    return raw_humidity;
}

// MS5607
static void ms5607_send_command(uint8_t cmd) {
    reset_i2c_bus();
    I2C_setTargetAddress(I2CA_BASE, PRESSURE_ADDR);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_sendStartCondition(I2CA_BASE);

    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_TX_DATA_RDY));
    I2C_putData(I2CA_BASE, cmd);

    I2C_sendStopCondition(I2CA_BASE);
    while (I2C_isBusBusy(I2CA_BASE));
}

static uint32_t ms5607_read_adc(void) {
    uint8_t cmd = 0x00; // ADC_READ command

    reset_i2c_bus(); // Optional but useful
    DEVICE_DELAY_US(100);

    // --- Send command 0x00 to trigger ADC read ---
    I2C_setTargetAddress(I2CA_BASE, PRESSURE_ADDR);
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_putData(I2CA_BASE, cmd);
    I2C_sendStartCondition(I2CA_BASE);

    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_STOP_CONDITION) &&
           !(I2C_getStatus(I2CA_BASE) & I2C_STS_TX_DATA_RDY));
    while (I2C_isBusBusy(I2CA_BASE));

    DEVICE_DELAY_US(500);  // Wait a bit before reading

    // --- Now read 3 bytes from ADC ---
    I2C_setTargetAddress(I2CA_BASE, PRESSURE_ADDR);
    I2C_setDataCount(I2CA_BASE, 3);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE);
    I2C_sendStartCondition(I2CA_BASE);

    uint32_t adc = 0;
    for (int i = 0; i < 3; i++) {
        int timeout = 100000;
        while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY)) {
            if (--timeout == 0) {
                I2C_sendStopCondition(I2CA_BASE);
                return 0xFFFFFFFF;
            }
        }
        adc = (adc << 8) | I2C_getData(I2CA_BASE);
    }

    I2C_sendStopCondition(I2CA_BASE);
    while (I2C_isBusBusy(I2CA_BASE));

    return adc;
}


static uint16_t ms5607_read_prom(uint8_t index) {
    uint8_t cmd = CMD_PROM_READ_BASE + (index * 2);

    reset_i2c_bus();
    DEVICE_DELAY_US(100);

    // WRITE PHASE (send command)
    I2C_setTargetAddress(I2CA_BASE, PRESSURE_ADDR);
    I2C_setDataCount(I2CA_BASE, 1); // Only sending 1 byte command
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_putData(I2CA_BASE, cmd);
    I2C_sendStartCondition(I2CA_BASE);  // Send START + address + command

    while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_TX_DATA_RDY));


    DEVICE_DELAY_US(500); // Small delay before reading

    // READ PHASE (read 2 bytes)
    I2C_setTargetAddress(I2CA_BASE, PRESSURE_ADDR);
    I2C_setDataCount(I2CA_BASE, 2);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE);
    I2C_sendStartCondition(I2CA_BASE);

    uint16_t value = 0;
    for (int i = 0; i < 2; i++) {
        int timeout = 100000;
        while (!(I2C_getStatus(I2CA_BASE) & I2C_STS_RX_DATA_RDY)) {
            if (--timeout == 0) {
                I2C_sendStopCondition(I2CA_BASE);
                return 0xFFFF;
            }
        }
        value = (value << 8) | I2C_getData(I2CA_BASE);
    }

    I2C_sendStopCondition(I2CA_BASE);
    while (I2C_isBusBusy(I2CA_BASE));

    return value;
}


void ms5607_init_once(void) {
    if (ms5607_initialized) return;

    ms5607_send_command(CMD_RESET);
    delay_ms(5);

    for (int i = 1; i <= 6; i++)
        C[i] = ms5607_read_prom(i);

    ms5607_send_command(CMD_CONVERT_D2_OSR);
    delay_ms(10);
    uint32_t D2 = ms5607_read_adc();
    dT_global = (int32_t)D2 - ((int32_t)C[5] << 8);

    ms5607_initialized = true;
}

float read_pressure_ms5607(void) {
    if (!ms5607_initialized) ms5607_init_once();

    ms5607_send_command(CMD_CONVERT_D1_OSR);
    delay_ms(10);
    uint32_t D1 = ms5607_read_adc();

    int64_t OFF  = ((int64_t)C[2] << 16) + ((int64_t)C[4] * dT_global) / (1 << 7);
    int64_t SENS = ((int64_t)C[1] << 15) + ((int64_t)C[3] * dT_global) / (1 << 8);
    int32_t P    = (((int64_t)D1 * SENS) / (1 << 21) - OFF) / (1 << 15);

    return P / 100.0f;
}
float read_temp_ms5607(void)
{
    if (!ms5607_initialized)
        ms5607_init_once();

    ms5607_send_command(CMD_CONVERT_D2_OSR);  // Start temperature conversion
    delay_ms(10);                             // Wait for conversion
    uint32_t D2 = ms5607_read_adc();          // Read raw D2 value

    dT_global = (int32_t)D2 - ((int32_t)C[5] << 8);  // Recalculate dT

    int32_t TEMP = 2000 + ((int64_t)dT_global * (int64_t)C[6]) / (1 << 23);
    return TEMP / 100.0f;
}
//---------------------------------- MAIN ----------------------------------

void main(void) {
    Device_init();
    Device_initGPIO();

    init_card();
    init_uart_rs422();
    init_I2C_perip();


    while (1) {
        if (SCI_getRxFIFOStatus(SCIC_BASE) >= 2) {  // wait until at least 2 bytes received
        uint8_t high_byte = SCI_readCharBlockingFIFO(SCIC_BASE);
        uint8_t low_byte = SCI_readCharBlockingFIFO(SCIC_BASE);
        uint16_t command = (high_byte << 8) | low_byte;
        cmd_decoder(command);
        }
    }
}