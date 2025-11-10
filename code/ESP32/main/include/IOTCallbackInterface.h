#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>

class IOTCallbackInterface
{
public:
    virtual void onNetworkConnect() = 0;
    virtual void addApplicationConfigs(String &page);
    virtual void onSubmitForm(AsyncWebServerRequest *request);
    virtual void onSaveSetting(JsonDocument &doc);
    virtual void onLoadSetting(JsonDocument &doc);
};