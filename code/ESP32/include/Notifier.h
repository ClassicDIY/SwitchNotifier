#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include "log.h"
#include "Defines.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace SwitchNotifier
{

class Notifier : public IOTCallbackInterface
{
 public:
 	Notifier();
	~Notifier();
    void setup(IOTServiceInterface* pcb);
    void notify(uint8_t pin);

    //IOTCallbackInterface 
    String getRootHTML() ;
    iotwebconf::ParameterGroup* parameterGroup() ;
    bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper) ;

 protected:
    void sendit(const char * content);
    SMTPSession* _smtp;
    IOTServiceInterface* _pcb;
};

} // namespace SwitchNotifier
