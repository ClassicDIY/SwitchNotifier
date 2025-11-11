#pragma once
#include "Defines.h"

unsigned long inline getTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return (0);
    }
    time(&now);
    return now;
}

template <typename T> String htmlConfigEntry(const char *label, T val) {
    String s = "<li>";
    s += label;
    s += ": ";
    s += val;
    s += "</li>";
    return s;
}

void inline light_sleep(uint32_t sec) {
    esp_sleep_enable_timer_wakeup(sec * 1000000ULL);
    esp_light_sleep_start();
}