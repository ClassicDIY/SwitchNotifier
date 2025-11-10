#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"
#include <ESP_Mail_Client.h>

class IOTServiceInterface
{
public:
    virtual u_int getUniqueId() = 0;
    virtual std::string getThingName() = 0;
    #if TINY_GSM_MODEM_SIM7600
    virtual void setGSMClient(SMTPSession* smtpSession) = 0;
    #endif
};