# Motor Drive DSP Sensor Suite

Firmware and project scaffolding for a TI CCS (Theia) based DSP application that acquires temperature, pressure, and humidity from TMP100, MS5607, and HIH8131 sensors on a motor drive card. Includes initialization, periodic sampling, data validation, unit conversion, and streamlined hooks for diagnostics and downstream control loops.

Suggested repository name: motor-drive-dsp-sensor-suite

## Highlights
- **Sensors**:
  - TMP100: Ambient temperature over I²C (12‑bit typical)
  - MS5607: Barometric pressure and temperature over I²C/SPI (this project assumes I²C unless noted)
  - HIH8131: Relative humidity and temperature over I²C
- **DSP‑oriented**: Deterministic sampling windows, fixed‑point friendly conversions, low‑latency ISR/RTOS integration
- **Motor‑drive context**: Noise‑aware bus timing, EMC‑conscious pull‑ups, optional filtering for PWM‑rich environments
- **Pluggable outputs**: Debug UART/SCI, structured logs, in‑memory ring buffer for control tasks

## Repository structure
- `RS422_UART_Test/`: Project folder reused as the CCS (Theia) project container for the DSP firmware
- `.theia/`: Workspace settings for CCS (Theia)
- `.vscode/`: Optional editor configuration

Tip: The logical purpose of `RS422_UART_Test/` is now sensor acquisition; you may rename it in CCS to match this README (e.g., `Sensor_Acq_DSP`) once imports are stable.

## Requirements

### Hardware
- TI DSP/MCU on your motor drive card (e.g., C2000 series or similar)
- I²C bus with appropriate pull‑ups (2.2–4.7 kΩ typical) routed to:
  - TMP100 (addr selectable via A0)
  - MS5607 (I²C mode) or ensure SPI wiring if using SPI variant
  - HIH8131 (fixed I²C address)
- Decoupling caps near each sensor; respect layout guidance from datasheets
- Stable 3.3 V (or as required) and common ground

### Software
- TI Code Composer Studio (Theia)
- Device support packages and SDK for your DSP/MCU
- Optional: Serial terminal for debug telemetry

## Quick start

1. Open TI CCS (Theia).
2. Import project:
   - File → Import… → CCS Projects → Select workspace → choose `RS422_UART_Test/`
3. Configure target:
   - Device/CPU, compiler, SDK paths
   - I²C peripheral instance and pins (SCL/SDA)
   - Optional debug UART instance and pins
4. Build and flash.
5. Open a serial terminal (if enabled) to observe sensor readings and status.

## Sensor interfaces

- TMP100 (I²C):
  - Address: 0x48–0x4B depending on A0
  - Typical config: 12‑bit resolution, continuous conversion
  - Conversion: temp_celsius = raw / 16.0 (12‑bit left‑aligned)
- MS5607 (I²C assumed):
  - Address: 0x76 or 0x77
  - Requires PROM coefficient read on startup
  - Use OSR setting to trade conversion time vs. noise; compute compensated pressure and temperature per datasheet
- HIH8131 (I²C):
  - Address: 0x27 (typical)
  - After measurement trigger/read: convert to %RH and °C per transfer function

## Timing and scheduling

- Default sampling period: 10–100 ms (configure per application)
- Sequence:
  1. Trigger MS5607 conversions (D1 pressure / D2 temperature) with desired OSR
  2. Read TMP100 temperature
  3. Read HIH8131 humidity/temperature
  4. Collect MS5607 results and compute compensated values
- Use a timer ISR or RTOS task with bounded execution; avoid I²C bus contention with other peripherals.

## Data validation and units

- TMP100: clamp to plausible ambient range (e.g., −40 to 125 °C)
- MS5607: CRC check on PROM; reject out‑of‑range pressures and temps
- HIH8131: status bits for stale/error data; clamp 0–100 %RH
- Export in SI units: °C, kPa (or mbar), %RH

## Configuration

Common configuration points inside the project folder:
- I²C peripheral index, GPIO pins, bus speed (e.g., 100 kHz or 400 kHz)
- MS5607 OSR selection and measurement cadence
- TMP100 resolution/alert config if used
- HIH8131 measurement mode
- Telemetry enable (UART) and baud rate
- Fixed‑point vs. floating‑point conversions

Consider centralizing in a header such as `sensor_config.h`.

## Diagnostics

- Optional UART log: one line per sample window, e.g. `T=24.31C, P=101.42kPa, RH=45.6%`
- Error counters for I²C NACK, CRC failure, out‑of‑range data
- Ring buffer for last N samples to aid control algorithm debugging

## Troubleshooting

- Bus not responding:
  - Verify pull‑ups and voltage levels; check that addresses match strap options
  - Probe SCL/SDA with a scope; confirm rise times and no bus holds
- Noisy/erratic readings on motor drive:
  - Shorten I²C traces, add series resistors (22–47 Ω), adjust sampling window away from PWM edges
  - Increase MS5607 OSR or apply light digital filtering
- Conversion math off:
  - Re‑read MS5607 PROM and verify CRC; confirm endianness and scaling

## Contributing

- Use feature branches and conventional commits
- Include device/board specifics and CCS version in PRs
- Provide scope captures or logs for interface‑level issues

## License

Add a license file (MIT recommended) at repository root and update this section.

## Maintainers / Support

When filing issues, include:
- DSP/MCU part number and board revision
- CCS (Theia) and SDK versions
- I²C wiring details and pull‑up values
- Any relevant scope captures or logs 
