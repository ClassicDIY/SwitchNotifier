#pragma once
#include <Arduino.h>

class MQTTCommandInterface
{
public:
    virtual bool handleCommand(char *payload, size_t len) = 0;

};