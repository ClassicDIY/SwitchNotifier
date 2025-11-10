#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"

class Button {
public:
    gpio_num_t pin;
    std::string message;
    volatile bool pressed = false;
    int64_t lastTriggerTime = 0;

    Button(gpio_num_t p, const std::string& msg) : pin(p), message(msg) {}

    void init() {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << pin);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);

        gpio_isr_handler_add(pin, Button::gpio_isr_handler, this);
    }

    void handleInterrupt() {
        int64_t now = esp_timer_get_time();
        if (now - lastTriggerTime > 50) {
            lastTriggerTime = now;
            pressed = true;
        }
    }

    static void IRAM_ATTR gpio_isr_handler(void* arg) {
        Button* btn = static_cast<Button*>(arg);
        btn->handleInterrupt();
    }
};