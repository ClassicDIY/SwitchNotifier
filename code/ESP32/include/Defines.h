
#pragma once

#define TAG "SwitchNotifier"
#define CONFIG_VERSION "V1.0.3" // major.minor.build (major or minor will invalidate the configuration)
#define HOME_ASSISTANT_PREFIX "homeassistant" // MQTT prefix used in autodiscovery
#define STR_LEN 255                            // general string buffer size
#define CONFIG_LEN 32                         // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define MQTT_TOPIC_LEN 128
#define AP_TIMEOUT 30000
#define MAX_PUBLISH_RATE 30000
#define MIN_PUBLISH_RATE 1000

#define WAKE_PUBLISH_RATE 2000
#define SNOOZE_PUBLISH_RATE 300000
#define WAKE_COUNT 60
#define WATCHDOG_TIMER 600000 //time in ms to trigger the watchdog

#define SMTP_server "smtp.gmail.com"
#define SMTP_Port 465
#define TZ "EST5EDT,M3.2.0,M11.1.0"
#define NTP_Server1 "pool.ntp.org"
#define NTP_Server2 "time.nist.gov"


#if TINY_GSM_MODEM_SIM7600
// SIM7600G PIN assignments
#define UART_BAUD           115200

#define MODEM_TX            27
#define MODEM_RX            26
#define MODEM_PWRKEY        4
#define MODEM_DTR           32
#define MODEM_RI            33
#define MODEM_FLIGHT        25
#define MODEM_STATUS        34

#define BUTTON_1 02
#define BUTTON_2 15
#define BUTTON_3 14
#define BUTTON_4 13

#define WIFI_STATUS_PIN 12
#define FACTORY_RESET_PIN 0

#else // regular ESP32 (no GSM)

#define BUTTON_1 21
#define BUTTON_2 19
#define BUTTON_3 32
#define BUTTON_4 33

#define WIFI_STATUS_PIN 2
#define FACTORY_RESET_PIN 4
#endif

