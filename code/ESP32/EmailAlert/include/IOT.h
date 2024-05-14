#pragma once

#include "WiFi.h"
#include "ArduinoJson.h"
#include <EEPROM.h>

#include "Log.h"
#include "AsyncMqttClient.h"
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <IotWebConfESP32HTTPUpdateServer.h>
#include "Defines.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"
#include "MQTTCallbackInterface.h"

namespace SwitchNotifier
{
class IOT : public IOTServiceInterface
{
public:
    void Init(IOTCallbackInterface* iotCB, MQTTCallbackInterface* cmdCB);
    boolean Run();
    void Publish(const char *subtopic, const char *value, boolean retained = false);
    void Publish(const char *topic, float value, boolean retained = false);
    void PublishMessage(const char* topic, JsonDocument& payload);
    void PublishTelemetery(bool online);
    u_int getUniqueId() { return _uniqueId;};
    const char* getThingName();
    const char* getDeviceName();
    IOTCallbackInterface* IOTCB() { return _iotCB;}

    bool ProcessCmnd(char *payload, size_t len);

private:
    MQTTCallbackInterface* _cmdCB;
    IOTCallbackInterface* _iotCB;
    bool _MQTTConfigured = false;
    bool _lastTelemetery = true; 
    u_int _uniqueId = 0; // unique id from mac address NIC segment
};


} // namespace SwitchNotifier

extern SwitchNotifier::IOT _iot;
