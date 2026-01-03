#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
// #include <LittleFS.h>
// #define ESP_MAIL_DEFAULT_FLASH_FS LittleFS
#include <ESPAsyncWebServer.h>
#include <ESP_Mail_Client.h>
#include "Defines.h"
#include "IOT_Defines.h"
#include "Device.h"
#include "IOTEnumerations.h"
#include "log.h"
#include "Button.h"
#include "IOTCallbackInterface.h"
#include "IOTServiceInterface.h"
#include "IDisplayServiceInterface.h"

class Notifier : public Device, public IOTCallbackInterface {
 public:
   Notifier();
   ~Notifier();
   void setup();
   void run();

   // IOTCallbackInterface
   void onNetworkState(NetworkState state);
   void onSocketPong();
   void onSaveSetting(JsonDocument &doc);
   void onLoadSetting(JsonDocument &doc);
   String appTemplateProcessor(const String &var);
#ifdef Has_OLED
   IDisplayServiceInterface& getDisplayInterface() override {  return _oled; };
#endif

 protected:
   SMTPSession *_smtp;
   void sendit(const String &content);
   String _smtpServer = SMTP_server;
   uint16_t _smtpPort = SMTP_Port;
   String _senderEmail = "joe@gmail.com";
   String _senderPassword = "secret";
   String _recipientEmail = "jane@gmail.com";
   String _recipientName = "tarzan";
   String _subject = "Switch Notifier";
#ifdef LILYGO_T_SIM7600G
   Button _buttons[NUM_BUTTONS] = {Button(DI0), Button(DI1),Button(DI2), Button(DI3),Button(DI4), Button(DI5),Button(DI6), Button(DI7)};
#else
   Button _buttons[NUM_BUTTONS] = {Button(DI0), Button(DI1), Button(DI2), Button(DI3), Button(DI4), Button(DI5), Button(DI6), Button(DI7)};
#endif
 private:
   String _bodyBuffer;
};

