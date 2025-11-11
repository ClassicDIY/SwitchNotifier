#include <Arduino.h>
#include <SPI.h>
#ifdef Has_OLED_Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif
#include "Defines.h"
#include "Log.h"
#include "Notifier.h"

using namespace CLASSICDIY;

Notifier _notifier = Notifier();
hw_timer_t *_watchdogTimer = NULL;
#ifdef Has_OLED_Display
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

void IRAM_ATTR resetModule() {
    // ets_printf("watchdog timer expired - rebooting\n");
    esp_restart();
}

void init_watchdog() {
    if (_watchdogTimer == NULL) {
        _watchdogTimer = timerBegin(0, 80, true);                      // timer 0, div 80
        timerAttachInterrupt(_watchdogTimer, &resetModule, true);      // attach callback
        timerAlarmWrite(_watchdogTimer, WATCHDOG_TIMER * 1000, false); // set time in us
        timerAlarmEnable(_watchdogTimer);                              // enable interrupt
    }
}

void feed_watchdog() {
    if (_watchdogTimer != NULL) {
        timerWrite(_watchdogTimer, 0); // feed the watchdog
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
    }
    logd("Booting");
    _notifier.setup();
    init_watchdog();
    logd("Done setup");
}

void loop() {
    _notifier.run();
    feed_watchdog(); // don't reset when not configured
}
