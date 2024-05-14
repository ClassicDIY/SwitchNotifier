#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>

class IOTCallbackInterface
{
public:
    virtual String getRootHTML() = 0;
    virtual iotwebconf::ParameterGroup* parameterGroup() = 0;
    virtual bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper) = 0;
};