#pragma once

#include "WiFi.h"
#include "ArduinoJson.h"
#include <EEPROM.h>

#include "Log.h"
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <IotWebConfESP32HTTPUpdateServer.h>
#include "Defines.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"


namespace SwitchNotifier
{
class IOT : public IOTServiceInterface
{
public:
    void Init(IOTCallbackInterface* iotCB);
    boolean Run();
    u_int getUniqueId() { return _uniqueId;};
    const char* getThingName();
    const char* getDeviceName();
    IOTCallbackInterface* IOTCB() { return _iotCB;}
    bool ProcessCmnd(char *payload, size_t len);
    #if TINY_GSM_MODEM_SIM7600
    void setGSMClient(SMTPSession* smtpSession);
    #endif

private:
    IOTCallbackInterface* _iotCB;
    u_int _uniqueId = 0; // unique id from mac address NIC segment
};


} // namespace SwitchNotifier

extern SwitchNotifier::IOT _iot;
