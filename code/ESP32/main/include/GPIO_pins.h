#pragma once
#include <Arduino.h>

#ifdef EDGEBOX

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_43
#define U0_RXD GPIO_NUM_44

// I2C
#define I2C_SDA GPIO_NUM_20
#define I2C_SCL GPIO_NUM_19

// I2C INT fro RTC PCF8563
#define I2C_INT GPIO_NUM_9;

// SPI BUS for W5500 Ethernet Port Driver
#define ETH_SS GPIO_NUM_10
#define ETH_MOSI GPIO_NUM_12
#define ETH_MISO GPIO_NUM_11
#define ETH_SCK GPIO_NUM_13
#define ETH_INT GPIO_NUM_14
#define ETH_RST GPIO_NUM_15

// CAN BUS
#define CAN_TXD GPIO_NUM_1
#define CAN_RXD GPIO_NUM_2

// BUZZER
#define BUZZER GPIO_NUM_45

#define DO0 GPIO_NUM_40
#define DO1 GPIO_NUM_39
#define DO2 GPIO_NUM_38
#define DO3 GPIO_NUM_37
#define DO4 GPIO_NUM_36
#define DO5 GPIO_NUM_35

#define DI0 GPIO_NUM_4
#define DI1 GPIO_NUM_5
#define DI2 GPIO_NUM_6
#define DI3 GPIO_NUM_7

// Analog Input (SGM58031) channels
#define AI0 = 0;
#define AI1 = 1;
#define AI2 = 2;
#define AI3 = 3;

// Analog Output
#define AO0 GPIO_NUM_42
#define AO1 GPIO_NUM_41

#elif NORVI_GSM_AE02

#define NUM_BUTTONS 8 // Number of digital input buttons

#define BUTTONS GPIO_NUM_36 // Analog pin to read buttons

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_3
#define U0_RXD GPIO_NUM_1

// I2C
#define I2C_SDA GPIO_NUM_16
#define I2C_SCL GPIO_NUM_17

#define DO0 GPIO_NUM_12
#define DO1 GPIO_NUM_2

#define DI0 GPIO_NUM_27
#define DI1 GPIO_NUM_34
#define DI2 GPIO_NUM_35
#define DI3 GPIO_NUM_14
#define DI4 GPIO_NUM_13
#define DI5 GPIO_NUM_5
#define DI6 GPIO_NUM_15
#define DI7 GPIO_NUM_19

// Analog Input (ADS1115) channels
#define AI0 = 0;
#define AI1 = 1;
#define AI2 = 2;
#define AI3 = 3;

// No Analog output

// OLED display definitions
#define SCREEN_ADDRESS 0x3C // OLED 128X64 I2C address
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

#elif LILYGO_T_SIM7600G

#define NUM_BUTTONS 8 // Number of digital input buttons

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_01
#define U0_RXD GPIO_NUM_03

// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

// digital inputs
#define DI0 GPIO_NUM_5
#define DI1 GPIO_NUM_13
#define DI2 GPIO_NUM_14
#define DI3 GPIO_NUM_15
#define DI4 GPIO_NUM_2
#define DI5 GPIO_NUM_23
#define DI6 GPIO_NUM_19
#define DI7 GPIO_NUM_18

#elif ESP32_Dev

// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

#define NUM_BUTTONS 8 // Number of digital input buttons
#define DI0 GPIO_NUM_13
#define DI1 GPIO_NUM_12
#define DI2 GPIO_NUM_14
#define DI3 GPIO_NUM_27
#define DI4 GPIO_NUM_26
#define DI5 GPIO_NUM_25
#define DI6 GPIO_NUM_33
#define DI7 GPIO_NUM_32

#elif ESP32_S3

// I2C
#define I2C_SDA GPIO_NUM_8
#define I2C_SCL GPIO_NUM_9

#define NUM_BUTTONS 8 // Number of digital input buttons
#define DI0 GPIO_NUM_4
#define DI1 GPIO_NUM_5
#define DI2 GPIO_NUM_6
#define DI3 GPIO_NUM_7
#define DI4 GPIO_NUM_15
#define DI5 GPIO_NUM_16
#define DI6 GPIO_NUM_17
#define DI7 GPIO_NUM_18

#endif
