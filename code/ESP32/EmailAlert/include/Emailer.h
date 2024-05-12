#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "Defines.h"
#include "IOTCallbackInterface.h"

namespace EmailAlert
{

class Emailer
{
 public:
 	Emailer();
	~Emailer();
    void setup(IOTCallbackInterface* pcb);
    void sendit(const char * content);

 protected:
    SMTPSession* _smtp;
    IOTCallbackInterface* _pcb;
};

} // namespace EmailAlert
