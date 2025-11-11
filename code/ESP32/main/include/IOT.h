#pragma once
#include "ArduinoJson.h"
#include "Defines.h"
#include "Enumerations.h"
#include "IOTCallbackInterface.h"
#include "IOTServiceInterface.h"
#include "OTA.h"
#include "time.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <sstream>
#include <string>

namespace CLASSICDIY {
class IOT : public IOTServiceInterface {
  public:
    IOT() {};
    void Init(IOTCallbackInterface *iotCB, AsyncWebServer *pwebServer);
    void Run();
    u_int getUniqueId() { return _uniqueId; };
    std::string getThingName();
    NetworkState getNetworkState() { return _networkState; }
    void GoOnline();
#if HasLTE
    void setGSMClient(SMTPSession *smtpSession);
#endif
  private:
    OTA _OTA = OTA();
    AsyncWebServer *_pwebServer;
    NetworkState _networkState = Boot;
    NetworkSelection _NetworkSelection = NotConnected;
    bool _blinkStateOn = false;
    String _AP_SSID = TAG;
    String _AP_Password = DEFAULT_AP_PASSWORD;
    bool _AP_Connected = false;
    String _SSID;
    String _WiFi_Password;
    String _APN;
    String _SIM_Username;
    String _SIM_Password;
    String _SIM_PIN;
    bool _useDHCP = false;
    char _Current_IP[16] = "";
    String _Static_IP;
    String _Subnet_Mask;
    String _Gateway_IP;
    uint32_t _settingsChecksum = 0;
    bool _needToReboot = false;

    IOTCallbackInterface *_iotCB;
    u_int _uniqueId = 0; // unique id from mac address NIC segment
    unsigned long _lastBlinkTime = 0;
    unsigned long _lastBootTimeStamp = millis();
    unsigned long _waitInAPTimeStamp = millis();
    unsigned long _NetworkConnectionStart = 0;
    unsigned long _FlasherIPConfigStart = millis();
    char _willTopic[STR_LEN * 2];
    char _rootTopicPrefix[STR_LEN];
    void RedirectToHome(AsyncWebServerRequest *request);
    void UpdateOledDisplay();
    void GoOffline();
    void saveSettings();
    void loadSettings();
    void setState(NetworkState newState);
    esp_err_t ConnectEthernet();
    void DisconnectEthernet();

#ifdef HasEthernet
    void HandleIPEvent(int32_t event_id, void *event_data);
    static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        IOT *instance = static_cast<IOT *>(arg);
        if (instance) {
            instance->HandleIPEvent(event_id, event_data);
        }
    }
#endif
};

} // namespace CLASSICDIY
