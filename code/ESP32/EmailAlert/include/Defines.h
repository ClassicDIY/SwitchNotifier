
#pragma once

#define TAG "EventAlert"
#define CONFIG_VERSION "V1.0.1" // major.minor.build (major or minor will invalidate the configuration)
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
#define sender_email "rossg256@gmail.com"
#define sender_password "dsow wcrh qthq cmhy"
#define Recipient_email "graham.a.ross@gmail.com"
#define Recipient_name "GR"
#define TZ "EST5EDT,M3.2.0,M11.1.0"
#define NTP_Server1 "pool.ntp.org"
#define NTP_Server2 "time.nist.gov"

