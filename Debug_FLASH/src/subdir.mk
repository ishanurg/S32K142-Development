################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/clock.c \
../src/delay.c \
../src/gpio.c \
../src/i2c.c \
../src/main.c \
../src/spi.c \
../src/uart.c 

OBJS += \
./src/clock.o \
./src/delay.o \
./src/gpio.o \
./src/i2c.o \
./src/main.o \
./src/spi.o \
./src/uart.o 

C_DEPS += \
./src/clock.d \
./src/delay.d \
./src/gpio.d \
./src/i2c.d \
./src/main.d \
./src/spi.d \
./src/uart.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/clock.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


