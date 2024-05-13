#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"

class IOTServiceInterface
{
public:

    virtual void Publish(const char *subtopic, const char *value, boolean retained) = 0;
    virtual void Publish(const char *topic, float value, boolean retained) = 0;
    virtual void PublishMessage(const char* topic, JsonDocument& payload) = 0;
    virtual void PublishTelemetery(bool online) = 0;
    virtual u_int getUniqueId() = 0;
    virtual const char* getThingName() = 0;
    virtual const char* getDeviceName() = 0;

};