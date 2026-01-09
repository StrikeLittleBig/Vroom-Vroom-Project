################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ENSI/Src/ensi_uart.c 

OBJS += \
./Drivers/ENSI/Src/ensi_uart.o 

C_DEPS += \
./Drivers/ENSI/Src/ensi_uart.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ENSI/Src/%.o Drivers/ENSI/Src/%.su Drivers/ENSI/Src/%.cyclo: ../Drivers/ENSI/Src/%.c Drivers/ENSI/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/ENSI/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ENSI-2f-Src

clean-Drivers-2f-ENSI-2f-Src:
	-$(RM) ./Drivers/ENSI/Src/ensi_uart.cyclo ./Drivers/ENSI/Src/ensi_uart.d ./Drivers/ENSI/Src/ensi_uart.o ./Drivers/ENSI/Src/ensi_uart.su

.PHONY: clean-Drivers-2f-ENSI-2f-Src

