#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include "log.h"
#include "Defines.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace EmailAlert
{

class Emailer : public IOTCallbackInterface
{
 public:
 	Emailer();
	~Emailer();
    void setup(IOTServiceInterface* pcb);
    void notify(uint8_t PIN);

    //IOTCallbackInterface 
    String getRootHTML() ;
    iotwebconf::ParameterGroup* parameterGroup() ;
    bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper) ;

 protected:
    void sendit(const char * content);
    SMTPSession* _smtp;
    IOTServiceInterface* _pcb;
};

} // namespace EmailAlert
