#include <Arduino.h>
#include "esp_task_wdt.h"
#include <SPI.h>
#ifdef Has_OLED_Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif
#include "main.h"
#include "Defines.h"
#include "Log.h"
#include "Notifier.h"

using namespace CLASSICDIY;
static Main my_main;
Notifier _notifier = Notifier();
hw_timer_t *_watchdogTimer = NULL;
#ifdef Has_OLED_Display
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

esp_err_t Main::setup() {
#ifdef Waveshare_Relay_6CH
   delay(5000);
#else
   Serial.begin(115200);
   while (!Serial) {
   }
#endif
   esp_err_t ret = ESP_OK;

   logd("------------ESP32 specifications ---------------");
   logd("Chip Model: %s", ESP.getChipModel());
   logd("Chip Revision: %d", ESP.getChipRevision());
   logd("Number of CPU Cores: %d", ESP.getChipCores());
   logd("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
   logd("Flash Memory Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
   logd("Flash Frequency: %d MHz", ESP.getFlashChipSpeed() / 1000000);
   logd("Heap Size: %d KB", ESP.getHeapSize() / 1024);
   logd("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
   logd("------------ESP32 specifications ---------------");

   GPIO_Init();
#ifdef Has_OLED_Display
   if (!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      loge("SSD1306 allocation failed");
   } else {
      oled_display.clearDisplay();
   }
#endif
   _notifier.setup();
   ret = esp_task_wdt_init(60, true); // 60-second timeout, panic on timeout
   esp_task_wdt_add(NULL);
   logd("Setup Done");
   return ret;
}

void Main::loop() {
   _notifier.run();
   esp_task_wdt_reset(); // Feed the watchdog
}

extern "C" void app_main(void) {
   logi("Creating default event loop");
   // Initialize esp_netif and default event loop
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());
   logi("Initialising NVS");
   ESP_ERROR_CHECK(nvs_flash_init());
   logi("Calling my_main.setup()");
   ESP_ERROR_CHECK(my_main.setup());
   while (true) {
      my_main.loop();
   }
}
