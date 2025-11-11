#include <Arduino.h>
#include "esp_task_wdt.h"
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

void setup() {
	#ifdef Waveshare_Relay_6CH
	delay(5000);
	#else
	Serial.begin(115200);
	while (!Serial) {}
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
	if(!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
		loge("SSD1306 allocation failed");
	} else {
		oled_display.clearDisplay();

	}
	#endif
    _notifier.setup();
    esp_task_wdt_init(60, true);  // 60-second timeout, panic on timeout
    esp_task_wdt_add(NULL); 
	logd("Setup Done");
}

void loop() {
    _notifier.run();
     esp_task_wdt_reset();  // Feed the watchdog
}
