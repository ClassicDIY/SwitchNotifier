#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP_Mail_Client.h>
#include "log.h"
#include "Defines.h"
#include "Button.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace CLASSICDIY
{

    // struct Button
    // {
    //     const uint8_t PIN;
    //     bool pressed;
    //     String message;
    //     std::function<void(void)> intRoutine;
    //     unsigned long lastTriggerTime;

    // };

    class Notifier : public IOTCallbackInterface
    {
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
        Button _buttons[NUM_BUTTONS] = { Button(DI0, "Button 0"), Button(DI1, "Button 1"), Button(DI2, "Button 2"), Button(DI3, "Button 3"), Button(DI4, "Button 4"), Button(DI5, "Button 5"), Button(DI6, "Button 6"), Button(DI7, "Button 7") };
    };

} // namespace SwitchNotifier
