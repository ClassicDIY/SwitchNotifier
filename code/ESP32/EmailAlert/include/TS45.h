#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "Enumerations.h"
#include "IOTCallbackInterface.h"
#include "Defines.h"
#include "RTUCallbackInterface.h"
#include "MQTTCommandInterface.h"
#include "RTUClient.h"

namespace EmailAlert
{

class TS45 : public RTUCallbackInterface, public MQTTCommandInterface
{
    
public:
	TS45();
    ~TS45();
    void begin(IOTCallbackInterface* pcb);
    void run();
    
    //MQTTCommandInterface
    bool handleCommand(char *payload, size_t len);

    //RTUCallbackInterface
    void handleData(ModbusMessage msg, uint32_t token);
	void handleError(ModbusError error, uint32_t token);

 protected:
    void PublishDiscovery();
    void PublishEntity(const char *subtopic, const char *device_class, const char *state_class, const char *entityName, const char *unit_of_meas = "", const char *icon = "");

    IOTCallbackInterface* _pcb;
    RTUClient* _rtuClient;

private:
    uint8_t lastRead[2048];
    unsigned long _lastModbusResponseTimeStamp = 0;
    bool _discoveryPublished = false;
    bool _mqttReadingsAvailable = false;
    bool _boilerPlateInfoPublished = false;
    bool _boilerPlateInfoRead = false;
    bool _deviceInfoInfoRead = false;
    uint8_t _lastCoilRead = 0xFF;
    char _manufacturer[32];
    char _model[16];
    char _version[16];
    int _rtuDeviceIDErrorCount = 0;
    uint16_t _controlMode = 0;
    int _rtuSuccededResponseCount = 0;
};

} // namespace EmailAlert