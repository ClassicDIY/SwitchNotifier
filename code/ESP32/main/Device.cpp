#include <Arduino.h>
#include <memory>
#include "Wire.h"
#include <ArduinoJson.h>
#ifdef UseLittleFS
#include <LittleFS.h>
#endif
#include "Log.h"
#include "Device.h"

namespace CLASSICDIY {

void Device::InitCommon() {
   Wire.begin(I2C_SDA, I2C_SCL);
#ifdef UseLittleFS
   if (!LittleFS.begin()) {
      loge("LittleFS mount failed");
   }
#endif
#ifdef Has_OLED
   _oled.Init();
#endif
}

#ifdef ESP32_Dev

void Device::Init() {
   InitCommon();
   pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
   pinMode(WIFI_STATUS_PIN, OUTPUT);
}

void Device::Run() {
   // handle blink led, fast : NotConnected slow: AP connected On: Station connected
   if (_networkState != OnLine) {
      unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
      unsigned long now = millis();
      if (binkRate < now - _lastBlinkTime) {
         _blinkStateOn = !_blinkStateOn;
         _lastBlinkTime = now;
         digitalWrite(WIFI_STATUS_PIN, _blinkStateOn ? HIGH : LOW);
      }
   } else {
      digitalWrite(WIFI_STATUS_PIN, LOW);
   }
}

#endif

#ifdef ESP32_S3

void Device::Init() {
   InitCommon();
   pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
}

void Device::Run() {
   if (_networkState != OnLine) {
      unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
      unsigned long now = millis();
      if (binkRate < now - _lastBlinkTime) {
         _blinkStateOn = !_blinkStateOn;
         _lastBlinkTime = now;
         neopixelWrite(RGB_BUILTIN, _blinkStateOn ? 60 : 0, _blinkStateOn ? 0 : 60, 0);
      }
   } else if (!_running) {
      neopixelWrite(RGB_BUILTIN, 0, 0, 60);
      _running = true;
   }
}

#endif

#ifdef NORVI_GSM_AE02

void Device::Init() {
   InitCommon();
}

void Device::Run() {}

#endif
#ifdef LILYGO_T_SIM7600G

void Device::Init() {
   InitCommon();
   pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
   pinMode(WIFI_STATUS_PIN, OUTPUT);
}

void Device::Run() {
   // handle blink led, fast : NotConnected slow: AP connected On: Station connected
   if (_networkState != OnLine) {
      unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
      unsigned long now = millis();
      if (binkRate < now - _lastBlinkTime) {
         _blinkStateOn = !_blinkStateOn;
         _lastBlinkTime = now;
         digitalWrite(WIFI_STATUS_PIN, _blinkStateOn ? HIGH : LOW);
      }
   } else {
      digitalWrite(WIFI_STATUS_PIN, LOW);
   }
}

#endif

} // namespace CLASSICDIY