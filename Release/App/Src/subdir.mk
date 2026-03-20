################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/app.c \
../App/Src/gpio.c \
../App/Src/scpi.c \
../App/Src/uart.c \
../App/Src/uart_lib.c 

OBJS += \
./App/Src/app.o \
./App/Src/gpio.o \
./App/Src/scpi.o \
./App/Src/uart.o \
./App/Src/uart_lib.o 

C_DEPS += \
./App/Src/app.d \
./App/Src/gpio.d \
./App/Src/scpi.d \
./App/Src/uart.d \
./App/Src/uart_lib.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.c App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32L432xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../libscpi/inc -I../App/Inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src

clean-App-2f-Src:
	-$(RM) ./App/Src/app.cyclo ./App/Src/app.d ./App/Src/app.o ./App/Src/app.su ./App/Src/gpio.cyclo ./App/Src/gpio.d ./App/Src/gpio.o ./App/Src/gpio.su ./App/Src/scpi.cyclo ./App/Src/scpi.d ./App/Src/scpi.o ./App/Src/scpi.su ./App/Src/uart.cyclo ./App/Src/uart.d ./App/Src/uart.o ./App/Src/uart.su ./App/Src/uart_lib.cyclo ./App/Src/uart_lib.d ./App/Src/uart_lib.o ./App/Src/uart_lib.su

.PHONY: clean-App-2f-Src

