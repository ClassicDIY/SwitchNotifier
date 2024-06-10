#include "IOT.h"
#include <IotWebConfOptionalGroup.h>
#include <IotWebConfTParameter.h>
#include "HelperFunctions.h"

namespace SwitchNotifier
{

#if TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG Serial
// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS
#include <TinyGsmClient.h>
// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

TinyGsm gsm_modem(SerialAT);
TinyGsmClient gsm_client(gsm_modem);
#endif

bool _NTPConfigured = false;
bool _needReset = false;
bool _WIFIConnected = false;
String clientId; // MQTT client ID
WiFiClient net;
DNSServer _dnsServer;
WebServer _webServer(80);
HTTPUpdateServer _httpUpdater;
IotWebConf _iotWebConf(TAG, &_dnsServer, &_webServer, TAG, CONFIG_VERSION);
iotwebconf::ParameterGroup GSM_group = iotwebconf::ParameterGroup("gsm", "GSM");
iotwebconf::CheckboxTParameter useGSM = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("useGSM").label("Send notifications via GSM").defaultValue(false).build();
iotwebconf::TextTParameter<STR_LEN> pinGPRSParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("pinGPRSParam").label("GPRS PIN").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> apnGPRSParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("apnGPRSParam").label("GPRS APN").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> userGPRSParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("userGPRSParam").label("GPRS User").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> passGPRSParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("passGPRSParam").label("GPRS Password").defaultValue("").build();

void WifiConnectionCallback() {
	String s;
	JsonDocument doc;
	logd("Waiting for NTP server time reading");
	configTzTime(TZ, NTP_Server1, NTP_Server2);
	doc["IP"] = WiFi.localIP().toString();
	doc["ApPassword"] = TAG;
	serializeJson(doc, s);
	s += '\n';
	Serial.printf(s.c_str()); // send json to flash tool
	_WIFIConnected = true;
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot() {
	// -- Let IotWebConf test and handle captive portal requests.
	if (_iotWebConf.handleCaptivePortal())
	{
		// -- Captive portal request were already served.
		return;
	}
	String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>PylonToMQTT</title></head><body>";
	s += "<h2>";
	s += _iotWebConf.getThingName();
	s += "</h2><hr><p>";
	s += _iot.IOTCB()->getRootHTML();
	s += "</p>";
	s += "<p>Go to <a href='config'>configure page</a> to change values.</p>";
	s += "</body></html>\n";
	_webServer.send(200, "text/html", s);
}

void configSaved() {
	logi("Configuration was updated, will reboot!.");
	_needReset = true;
}

boolean formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper) {
	if (_iot.IOTCB()->validate(webRequestWrapper) == false) return false;
	return true;
}

void IOT::Init(IOTCallbackInterface* iotCB) {
	_iotCB = iotCB;
	pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
	_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
	// setup EEPROM parameters
	GSM_group.addItem(&useGSM);
	GSM_group.addItem(&pinGPRSParam);
	GSM_group.addItem(&apnGPRSParam);
	GSM_group.addItem(&userGPRSParam);
	GSM_group.addItem(&passGPRSParam);
	_iotWebConf.addParameterGroup(&GSM_group);
	_iotWebConf.addParameterGroup(_iotCB->parameterGroup());
	// setup callbacks for IotWebConf
	_iotWebConf.setConfigSavedCallback(&configSaved);
	_iotWebConf.setFormValidator(&formValidator);
	_iotWebConf.setupUpdateServer(
      [](const char* updatePath) { _httpUpdater.setup(&_webServer, updatePath); },
      [](const char* userName, char* password) { _httpUpdater.updateCredentials(userName, password); });
	if (digitalRead(FACTORY_RESET_PIN) == LOW)
	{
		EEPROM.begin(IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH );
		for (byte t = 0; t < IOTWEBCONF_CONFIG_VERSION_LENGTH; t++)
		{
			EEPROM.write(IOTWEBCONF_CONFIG_START + t, 0);
		}
		EEPROM.commit();
		EEPROM.end();
		_iotWebConf.resetWifiAuthInfo();
		logw("Factory Reset!");
	}
	_iotWebConf.setWifiConnectionCallback(WifiConnectionCallback);
	boolean validConfig = _iotWebConf.init();
	logd("_iotWebConf.init complete");
	if (!validConfig)
	{
		logw("!invalid configuration!, set default values");
		_iotWebConf.resetWifiAuthInfo();
	}
	else
	{
		logi("Valid configuration!");
		// _iotWebConf.goOffLine(); 
		#if TINY_GSM_MODEM_SIM7600
		if (useGSM.isChecked()) {
			logi("Setup GSM, WIFI will remain in AP mode for 30 seconds.");
			pinMode(MODEM_PWRKEY, OUTPUT);
			digitalWrite(MODEM_PWRKEY, HIGH);
			delay(1000); //Need delay
			digitalWrite(MODEM_PWRKEY, LOW);
			// MODEM_FLIGHT IO:25 Modulator flight mode control, need to enable modulator, this pin must be set to high
			pinMode(MODEM_FLIGHT, OUTPUT);
			digitalWrite(MODEM_FLIGHT, HIGH);
			delay(3000);
			SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
			logi("Initializing modem...");
			uint retryCount = 3;
			while (!gsm_modem.init())
			{
				logw("Failed to initialize modem, delaying 10s and retrying.");
				light_sleep(10);
				if (--retryCount == 0) {
					logw("Giving up!");
					return;
				}
			}
			// gsm_modem.setNetworkMode(2); //Automatic
			logi("Waiting for network...");
			retryCount = 3;
			if (!gsm_modem.waitForNetwork(600000L)) {
				logw("The module did not connect to the network even after waiting, delaying 10s and retrying.");
				light_sleep(10);
				if (--retryCount == 0) {
					logw("Giving up!");
					return;
				}
			}
			if (gsm_modem.isNetworkConnected()) {
				logi("Network connected");
			}
			// retryCount = 3;
			// if (!gsm_modem.gprsConnect(apnGPRSParam.value(), userGPRSParam.value(), passGPRSParam.value())) {
			// 	loge("gprsConnect failed to connect");
			// 	light_sleep(10);
			// 	if (--retryCount == 0) {
			// 		logw("Giving up!");
			// 		return;
			// 	}
			// }
			// if (gsm_modem.isGprsConnected()) {
			// 	logi("GPRS connected");
			// }
			// String cop = gsm_modem.getOperator();
			// IPAddress local = gsm_modem.localIP();
			// logi("Operator: %s Local IP: %s", cop.c_str(), local.toString());
		} else {
			_iotWebConf.skipApStartup();
		}
		#else
		_iotWebConf.skipApStartup();
		#endif
	}
	// generate unique id from mac address NIC segment
	uint8_t chipid[6];
	esp_efuse_mac_get_default(chipid);
	_uniqueId = chipid[3] << 16;
	_uniqueId += chipid[4] << 8;
	_uniqueId += chipid[5];
	
	// IotWebConfParameter* p = _iotWebConf.getApPasswordParameter();
	// logi("AP Password: %s", p->valueBuffer);
	// Set up required URL handlers on the web server.
	_webServer.on("/", handleRoot);
	_webServer.on("/config", [] { _iotWebConf.handleConfig(); });
	_webServer.onNotFound([]() { _iotWebConf.handleNotFound(); });
	logd("Setup done");
}

boolean IOT::Run() {
	bool rVal = false;
	_iotWebConf.doLoop();
	if (_needReset)	{
		logi("Configuration changed, Rebooting after 1 second.");
		_iotWebConf.delay(1000);
		ESP.restart();
	}
	if (_WIFIConnected) {
		if (_NTPConfigured == false) {
			struct tm timeinfo;
			if(!getLocalTime(&timeinfo)){
				loge("Failed to obtain time 1");
				return rVal;
			}
			Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
			_NTPConfigured = true;
		} else {
			rVal = _NTPConfigured; // not using MQTT
		}
	}
	#if TINY_GSM_MODEM_SIM7600
	else if (gsm_modem.isNetworkConnected()) {
		if (_NTPConfigured == false) {
			String dateTime = gsm_modem.getGSMDateTime(DATE_FULL);
			logi("GSM dateTime: %s", dateTime.c_str());
			_NTPConfigured = true;
		} else {
			rVal = _NTPConfigured; // not using MQTT
		}
	}
	#endif
	else {
		// set SSID/PW from flasher.exe app
		if (Serial.peek() == '{') {
			String s = Serial.readStringUntil('}');
			s += "}";
			JsonDocument doc;
			DeserializationError err = deserializeJson(doc, s);
			if (err) {
				loge("deserializeJson() failed: %s", err.c_str());
			}
			else {
				if (doc.containsKey("ssid") && doc.containsKey("password"))	{
					iotwebconf::Parameter *p = _iotWebConf.getWifiSsidParameter();
					strcpy(p->valueBuffer, doc["ssid"]);
					logd("Setting ssid: %s", p->valueBuffer);
					p = _iotWebConf.getWifiPasswordParameter();
					strcpy(p->valueBuffer, doc["password"]);
					logd("Setting password: %s", p->valueBuffer);
					p = _iotWebConf.getApPasswordParameter();
					strcpy(p->valueBuffer, TAG); // reset to default AP password
					_iotWebConf.saveConfig();
					esp_restart();
				}
				else {
					logw("Received invalid json: %s", s.c_str());
				}
			}
		}
		else {
			Serial.read(); // discard data
		}
	}
	return rVal;
}

const char* IOT::getThingName() {
	return _iotWebConf.getThingName();
}

#if TINY_GSM_MODEM_SIM7600
void IOT::setGSMClient(SMTPSession* smtpSession) {
	if (useGSM.isChecked()) {
		smtpSession->setGSMClient(&gsm_client, &gsm_modem, pinGPRSParam.value(), apnGPRSParam.value(), userGPRSParam.value(), passGPRSParam.value());
	}
}
#endif

} // namespace SwitchNotifier