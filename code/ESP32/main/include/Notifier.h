#pragma once
#include "Button.h"
#include "Defines.h"
#include "IOTCallbackInterface.h"
#include "IOTServiceInterface.h"
#include "log.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP_Mail_Client.h>

namespace CLASSICDIY {

// struct Button
// {
//     const uint8_t PIN;
//     bool pressed;
//     String message;
//     std::function<void(void)> intRoutine;
//     unsigned long lastTriggerTime;

// };

class Notifier : public IOTCallbackInterface {
  public:
    Notifier();
    ~Notifier();
    void setup();
    void run();

    // IOTCallbackInterface
    void onNetworkConnect() {};
    void addApplicationConfigs(String &page);
    void onSubmitForm(AsyncWebServerRequest *request);
    void onSaveSetting(JsonDocument &doc);
    void onLoadSetting(JsonDocument &doc);

  protected:
    void sendit(std::string content);
    SMTPSession *_smtp;
    String _smtpServer = SMTP_server;
    uint16_t _smtpPort = SMTP_Port;
    String _senderEmail = "joe@gmail.com";
    String _senderPassword = "secret";
    String _recipientEmail = "jane@gmail.com";
    String _recipientName = "tarzan";
    String _subject = "Switch Notifier";
    Button _buttons[NUM_BUTTONS] = {Button(DI0), Button(DI1), Button(DI2), Button(DI3), Button(DI4), Button(DI5), Button(DI6), Button(DI7)};
};

} // namespace CLASSICDIY
