#pragma once
#include <Arduino.h>

class MQTTCallbackInterface
{
public:
    virtual bool handleCommand(char *payload, size_t len) = 0;

};