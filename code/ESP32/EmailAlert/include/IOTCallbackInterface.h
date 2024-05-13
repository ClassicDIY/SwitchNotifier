#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"

class IOTCallbackInterface
{
public:

    virtual void Publish(const char *subtopic, const char *value, boolean retained) = 0;
    virtual void Publish(const char *topic, float value, boolean retained) = 0;
    virtual void PublishMessage(const char* topic, JsonDocument& payload) = 0;
    virtual void PublishTelemetery(bool online) = 0;
    virtual u_int getUniqueId() = 0;
    virtual const char* getThingName() = 0;
    virtual const char* getDeviceName() = 0;
    virtual const char* getSMTPServer() = 0;
    virtual uint16_t getSMTPPort() = 0;
    virtual const char* getSenderEmail() = 0;
    virtual const char* getSenderPassword() = 0;
    virtual const char* getRecipientEmail() = 0;
    virtual const char* getRecipientName() = 0;

};