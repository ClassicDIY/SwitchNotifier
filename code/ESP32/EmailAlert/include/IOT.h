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
#include "IOTCallbackInterface.h"
#include "MQTTCommandInterface.h"

namespace EmailAlert
{
class IOT : public IOTCallbackInterface
{
public:
    void Init(MQTTCommandInterface* cmdCB);
    boolean Run();
    void Publish(const char *subtopic, const char *value, boolean retained = false);
    void Publish(const char *topic, float value, boolean retained = false);
    void PublishMessage(const char* topic, JsonDocument& payload);
    void PublishTelemetery(bool online);
    u_int getUniqueId() { return _uniqueId;};
    const char* getThingName();
    const char* getDeviceName();
    const char* getSMTPServer();
    uint16_t getSMTPPort();
    const char* getSenderEmail();
    const char* getSenderPassword();
    const char* getRecipientEmail();
    const char* getRecipientName();
    bool ProcessCmnd(char *payload, size_t len);

private:
    MQTTCommandInterface* _cmdCB;
    bool _MQTTConfigured = false;
    bool _lastTelemetery = true; 
    u_int _uniqueId = 0; // unique id from mac address NIC segment
};


} // namespace EmailAlert

extern EmailAlert::IOT _iot;
