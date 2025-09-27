################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-1122822655: ../c2000.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2020/ccs/utils/sysconfig_1.24.0/sysconfig_cli.bat" --script "C:/Users/holym/workspace_ccstheia/RS422_UART_Test/c2000.syscfg" -o "syscfg" -s "C:/ti/C2000Ware_5_05_00_00/.metadata/sdk.json" -d "F2838x" -p "337bga" -r "F2838x_337bga" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-1122822655 ../c2000.syscfg
syscfg/board.h: build-1122822655
syscfg/board.cmd.genlibs: build-1122822655
syscfg/board.opt: build-1122822655
syscfg/board.json: build-1122822655
syscfg/pinmux.csv: build-1122822655
syscfg/c2000ware_libraries.cmd.genlibs: build-1122822655
syscfg/c2000ware_libraries.opt: build-1122822655
syscfg/c2000ware_libraries.c: build-1122822655
syscfg/c2000ware_libraries.h: build-1122822655
syscfg/clocktree.h: build-1122822655
syscfg: build-1122822655

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu64 --idiv_support=idiv0 --tmu_support=tmu0 --vcu_support=vcrc -Ooff --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test" --include_path="C:/ti/C2000Ware_5_05_00_00" --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test/device" --include_path="C:/ti/C2000Ware_5_05_00_00/driverlib/f2838x/driverlib/" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=RAM --define=DEBUG --define=CPU1 --c99 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test/CPU1_RAM/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu64 --idiv_support=idiv0 --tmu_support=tmu0 --vcu_support=vcrc -Ooff --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test" --include_path="C:/ti/C2000Ware_5_05_00_00" --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test/device" --include_path="C:/ti/C2000Ware_5_05_00_00/driverlib/f2838x/driverlib/" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=RAM --define=DEBUG --define=CPU1 --c99 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/holym/workspace_ccstheia/RS422_UART_Test/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


