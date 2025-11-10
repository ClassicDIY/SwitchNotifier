#pragma once
#include <Arduino.h>

#ifdef EDGEBOX
#define DI_PINS 4	// Number of digital input pins
#define DO_PINS 6	// Number of digital output pins
#define AI_PINS 4	// Number of analog input pins
#define AO_PINS 2   // Number of analog output pins
#ifndef LOG_TO_SERIAL_PORT //disable logs to use LED wifi status
// use LED if the log level is none (edgeBox shares the LED pin with the serial TX gpio)
#define WIFI_STATUS_PIN 43 //LED Pin
#endif
#define FACTORY_RESET_PIN 2 // Clear NVRAM, shared with CAN_RXD

void inline GPIO_Init()
{

}

//Programming and Debugging Port
static const uint8_t U0_TXD = 43;
static const uint8_t U0_RXD = 44;

//I2C
static const uint8_t I2C_SDA = 20;
static const uint8_t I2C_SCL = 19;

//I2C INT fro RTC PCF8563
static const uint8_t I2C_INT = 9;

//SPI BUS for W5500 Ethernet Port Driver
static const uint8_t ETH_SS = 10;
static const uint8_t ETH_MOSI = 12;
static const uint8_t ETH_MISO = 11;
static const uint8_t ETH_SCK = 13;
static const uint8_t ETH_INT = 14;
static const uint8_t ETH_RST = 15;

//A7670G
static const uint8_t LTE_AIRPLANE_MODE = 16;
static const uint8_t LTE_PWR_EN = 21;
static const uint8_t LTE_TXD = 48;
static const uint8_t LTE_RXD = 47;

//RS485
static const uint8_t RS485_TXD = 17;
static const uint8_t RS485_RXD = 18;
static const uint8_t RS485_RTS = 8;

//CAN BUS
static const uint8_t CAN_TXD = 1;
static const uint8_t CAN_RXD = 2;

//BUZZER
static const uint8_t BUZZER = 45;

static const uint8_t DO0 = 40;
static const uint8_t DO1 = 39;
static const uint8_t DO2 = 38;
static const uint8_t DO3 = 37;
static const uint8_t DO4 = 36;
static const uint8_t DO5 = 35;

static const uint8_t DI0 = 4;
static const uint8_t DI1 = 5;
static const uint8_t DI2 = 6;
static const uint8_t DI3 = 7;

// Analog Input (SGM58031) channels
static const uint8_t AI0 = 0;
static const uint8_t AI1 = 1;
static const uint8_t AI2 = 2;
static const uint8_t AI3 = 3;

// Analog Output
static const uint8_t AO0 = 42;
static const uint8_t AO1 = 41;

#elif NORVI_GSM_AE02

#define DI_PINS 8	// Number of digital input pins
#define DO_PINS 2	// Number of digital output pins
#define AI_PINS 4	// Number of analog input pins
#define AO_PINS 0 // Number of analog output pins

static const uint8_t BUTTONS = 36; //Analog pin to read buttons

void inline GPIO_Init()
{
    pinMode(BUTTONS, INPUT);
}

//Programming and Debugging Port
static const uint8_t U0_TXD = 03;
static const uint8_t U0_RXD = 01;

//I2C
static const uint8_t I2C_SDA = 16;
static const uint8_t I2C_SCL = 17;

//GSM Modem
static const uint8_t LTE_PWR_EN = 21;
static const uint8_t LTE_TXD = 32;
static const uint8_t LTE_RXD = 33;

//RS485
static const uint8_t RS485_TXD = 26;
static const uint8_t RS485_RXD = 25;
static const uint8_t RS485_RTS = 22;

static const uint8_t DO0 = 12;
static const uint8_t DO1 = 2;

static const uint8_t DI0 = 27;
static const uint8_t DI1 = 34;
static const uint8_t DI2 = 35;
static const uint8_t DI3 = 14;
static const uint8_t DI4 = 13;
static const uint8_t DI5 = 5;
static const uint8_t DI6 = 15;
static const uint8_t DI7 = 19;

// Analog Input (ADS1115) channels
static const uint8_t AI0 = 0;
static const uint8_t AI1 = 1;
static const uint8_t AI2 = 2;
static const uint8_t AI3 = 3;

// No Analog output

#elif LILYGO_T_SIM7600G

#define DI_PINS 2	// Number of digital input pins
#define DO_PINS 2	// Number of digital output pins
#define AI_PINS 0	// Number of analog input pins
#define AO_PINS 0   // Number of analog output pins

#define WIFI_STATUS_PIN 12 //LED Pin
#define FACTORY_RESET_PIN 2 // Clear NVRAM

void inline GPIO_Init()
{

}

//Programming and Debugging Port
static const uint8_t U0_TXD = 01;
static const uint8_t U0_RXD = 03;

//I2C
static const uint8_t I2C_SDA = 21;
static const uint8_t I2C_SCL = 22;

//GSM Modem
static const uint8_t LTE_AIRPLANE_MODE = 25; // SIM7600G airplane mode pin, High to exit.
static const uint8_t LTE_PWR_EN = 4; // send power to the modem
static const uint8_t LTE_TXD = 27;
static const uint8_t LTE_RXD = 26;

// digital outputs
static const uint8_t DO0 = 14;
static const uint8_t DO1 = 15;

// digital inputs
static const uint8_t DI0 = 12;
static const uint8_t DI1 = 13;

#define BUTTON_1 02
#define BUTTON_2 15
#define BUTTON_3 14
#define BUTTON_4 13

#elif ESP32_Dev

#define WIFI_STATUS_PIN 2 //LED Pin
#define FACTORY_RESET_PIN 4 // Clear NVRAM

//I2C
static const uint8_t I2C_SDA = 21;
static const uint8_t I2C_SCL = 22;

#define NUM_BUTTONS 8	// Number of digital input buttons
#define DI0 GPIO_NUM_13
#define DI1 GPIO_NUM_12
#define DI2 GPIO_NUM_14
#define DI3 GPIO_NUM_27
#define DI4 GPIO_NUM_26
#define DI5 GPIO_NUM_25
#define DI6 GPIO_NUM_33
#define DI7 GPIO_NUM_32

#endif


