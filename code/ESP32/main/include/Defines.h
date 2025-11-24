
#pragma once

#include "GPIO_pins.h"

#define TAG "SwitchNotifier"

#define STR_LEN 255   // general string buffer size
#define CONFIG_LEN 32 // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define EEPROM_SIZE 2048
#define AP_BLINK_RATE 600
#define NC_BLINK_RATE 100

// #define AP_TIMEOUT 30000 //set back to 1000 in production
// #define AP_TIMEOUT 30000
#define AP_TIMEOUT 1000

#define FLASHER_TIMEOUT 10000
#define WS_CLIENT_CLEANUP 5000
#define WIFI_CONNECTION_TIMEOUT 120000
#define DEFAULT_AP_PASSWORD "12345678"

#define WATCHDOG_TIMER 600000 // time in ms to trigger the watchdog

#define SMTP_server "smtp.gmail.com"
#define SMTP_Port 465
#define TZ "EST5EDT,M3.2.0,M11.1.0"
#define NTP_Server1 "pool.ntp.org"
#define NTP_Server2 "time.nist.gov"

#define ASYNC_WEBSERVER_PORT 80
#define DNS_PORT 53
