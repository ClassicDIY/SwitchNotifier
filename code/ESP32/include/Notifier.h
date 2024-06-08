#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <ESP_Mail_Client.h>
#include <LittleFS.h>
#define ESP_MAIL_DEFAULT_FLASH_FS LittleFS
#include "log.h"
#include "Defines.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace SwitchNotifier
{

struct Button {
	const uint8_t PIN;
	bool pressed;
};

class Notifier : public IOTCallbackInterface
{
 public:
 	Notifier();
	~Notifier();
    void setup(IOTServiceInterface* pcb);
    void run();
    void notify(uint8_t pin);
    void monitorButton(Button& button);
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
