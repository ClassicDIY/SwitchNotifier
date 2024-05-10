#include "IOT.h"

namespace EmailAlert
{
bool _NTPConfigured = false;
AsyncMqttClient _mqttClient;
TimerHandle_t mqttReconnectTimer;
DNSServer _dnsServer;
WebServer _webServer(80);
HTTPUpdateServer _httpUpdater;
IotWebConf _iotWebConf(TAG, &_dnsServer, &_webServer, TAG, CONFIG_VERSION);
char _deviceName[IOTWEBCONF_WORD_LEN];
char _mqttServer[IOTWEBCONF_WORD_LEN];
char _mqttPort[NUMBER_CONFIG_LEN];
char _mqttUserName[IOTWEBCONF_WORD_LEN];
char _mqttUserPassword[IOTWEBCONF_WORD_LEN];
char _publishRateStr[NUMBER_CONFIG_LEN];
char _willTopic[MQTT_TOPIC_LEN];
char _rootTopicPrefix[MQTT_TOPIC_LEN];
iotwebconf::ParameterGroup Device_group = iotwebconf::ParameterGroup("DeviceGroup", "Device");
iotwebconf::TextParameter deviceNameParam = iotwebconf::TextParameter("Device Name", "CC1", _deviceName, IOTWEBCONF_WORD_LEN);
iotwebconf::ParameterGroup MQTT_group = iotwebconf::ParameterGroup("MQTT", "MQTT");
iotwebconf::TextParameter mqttServerParam = iotwebconf::TextParameter("MQTT server", "mqttServer", _mqttServer, IOTWEBCONF_WORD_LEN);
iotwebconf::NumberParameter mqttPortParam = iotwebconf::NumberParameter("MQTT port", "mqttSPort", _mqttPort, NUMBER_CONFIG_LEN, "text", NULL, "1883");
iotwebconf::TextParameter mqttUserNameParam = iotwebconf::TextParameter("MQTT user", "mqttUser", _mqttUserName, IOTWEBCONF_WORD_LEN);
iotwebconf::PasswordParameter mqttUserPasswordParam = iotwebconf::PasswordParameter("MQTT password", "mqttPass", _mqttUserPassword, IOTWEBCONF_WORD_LEN, "password");
iotwebconf::NumberParameter wakePublishRateParam = iotwebconf::NumberParameter("Publish Rate (S)", "wakePublishRate", _publishRateStr, NUMBER_CONFIG_LEN, "text", NULL, "2");

void onMqttConnect(bool sessionPresent)
{
	logd("Connected to MQTT. Session present: %d", sessionPresent);
	char buf[MQTT_TOPIC_LEN];
	sprintf(buf, "%s/cmnd/#", _rootTopicPrefix);
	_mqttClient.subscribe(buf, 0);
	_mqttClient.publish(_willTopic, 0, true, "Online", 6);
	logi("Subscribed to [%s], qos: 0", buf);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	logi("Disconnected from MQTT. Reason: %d", (int8_t)reason);
	if (WiFi.isConnected())
	{
		xTimerStart(mqttReconnectTimer, 0);
	}
	memset (_rootTopicPrefix, 0, MQTT_TOPIC_LEN);
}

void connectToMqtt()
{
	if (WiFi.isConnected())
	{
		if (strlen(_mqttServer) > 0) // mqtt configured
		{
			logi("Connecting to MQTT...");
			int len = strlen(_iotWebConf.getThingName());
			strncpy(_rootTopicPrefix, _iotWebConf.getThingName(), len);
			if (_rootTopicPrefix[len - 1] != '/')
			{
				strcat(_rootTopicPrefix, "/");
			}
			strcat(_rootTopicPrefix, _deviceName);

			sprintf(_willTopic, "%s/tele/LWT", _rootTopicPrefix);
			_mqttClient.setWill(_willTopic, 0, true, "Offline");
			_mqttClient.connect();
			logd("rootTopicPrefix: %s", _rootTopicPrefix);
		}
	}
}

void printLocalTime(){
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo)){
		loge("Failed to obtain time 1");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
	_NTPConfigured = true;
}

void WiFiEvent(WiFiEvent_t event)
{
	logd("[WiFi-event] event: %d", event);
	String s;
	JsonDocument doc;
	switch (event)
	{
	case SYSTEM_EVENT_STA_GOT_IP:
		logd("Waiting for NTP server time reading");
		configTzTime(TZ, NTP_Server1, NTP_Server2);
		printLocalTime();
		doc["IP"] = WiFi.localIP().toString();
		doc["ApPassword"] = TAG;
		serializeJson(doc, s);
		s += '\n';
		Serial.printf(s.c_str()); // send json to flash tool

		xTimerStart(mqttReconnectTimer, 0); // connect to MQTT once we have wifi
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		logi("WiFi lost connection");
		xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
		break;
	default:
		break;
	}
}

void onMqttPublish(uint16_t packetId)
{
	logi("Publish acknowledged.  packetId: %d", packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	logd("MQTT Message arrived [%s]  qos: %d len: %d index: %d total: %d", topic, properties.qos, len, index, total);
	printHexString(payload, len);
	bool messageProcessed = _iot.ProcessCmnd(payload, len);
	if (!messageProcessed) {
		JsonDocument doc;
		DeserializationError err = deserializeJson(doc, payload);
		if (err) // not json!
		{
			logd("MQTT payload {%s} is not valid JSON!", payload);
		}
		else 
		{
			boolean messageProcessed = false;
			if (doc.containsKey("wakePublishRate") && doc["wakePublishRate"].is<int>())
			{
				int publishRate = doc["wakePublishRate"];
				messageProcessed = true;
				if (publishRate >= MIN_PUBLISH_RATE && publishRate <= MAX_PUBLISH_RATE)
				{
					_iot.SetPublishRate(doc["wakePublishRate"]);
					logd("Wake publish rate: %d", _iot.PublishRate());
				}
				else
				{
					logd("wakePublishRate is out of rage!");
				}
			}
			if (!messageProcessed)
			{
				logd("MQTT Json payload {%s} not recognized!", payload);
			}
		}
	}
}

IOT::IOT()
{
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
	// -- Let IotWebConf test and handle captive portal requests.
	if (_iotWebConf.handleCaptivePortal())
	{
		// -- Captive portal request were already served.
		return;
	}
	String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>PylonToMQTT</title></head><body>";
	s += _iotWebConf.getThingName();
	s += "<ul>";
	s += "<li>MQTT server: ";
	s += _mqttServer;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT port: ";
	s += _mqttPort;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT user: ";
	s += _mqttUserName;
	s += "</ul>";
	s += "<ul>";
	s += "<li>Publish Rate: ";
	s += _publishRateStr;
	s += " (S)</ul>";
	s += "</ul>";
	s += "<ul>";
	s += "<li>Device Name: ";
	s += _deviceName;
	s += "</ul>";
	s += "Go to <a href='config'>configure page</a> to change values.";
	s += "</body></html>\n";
	_webServer.send(200, "text/html", s);
}

void configSaved()
{
	logi("Configuration was updated.");
}

boolean formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper)
{
	boolean valid = true;
	// int mqttServerParamLength = _webServer.arg(mqttServerParam.getId()).length();
	// if (mqttServerParamLength == 0)
	// {
	// 	mqttServerParam.errorMessage = "MQTT server is required";
	// 	valid = false;
	// }
	// int rate = _webServer.arg(wakePublishRateParam.getId()).toInt() * 1000;
	// if (rate < MIN_PUBLISH_RATE || rate > MAX_PUBLISH_RATE)
	// {
	// 	wakePublishRateParam.errorMessage = "Invalid publish rate.";
	// 	valid = false;
	// }
	return valid;
}

void IOT::Init(MQTTCommandInterface* cmdCB)
{
	_cmdCB = cmdCB;
	pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
	_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
	_iotWebConf.setConfigPin(WIFI_AP_PIN);
	// setup EEPROM parameters
	Device_group.addItem(&deviceNameParam);
   	MQTT_group.addItem(&mqttServerParam);
	MQTT_group.addItem(&mqttPortParam);
   	MQTT_group.addItem(&mqttUserNameParam);
	MQTT_group.addItem(&mqttUserPasswordParam);
	MQTT_group.addItem(&wakePublishRateParam);
	_iotWebConf.addParameterGroup(&Device_group);
	_iotWebConf.addParameterGroup(&MQTT_group);

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
	mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
	WiFi.onEvent(WiFiEvent);
	boolean validConfig = _iotWebConf.init();
	if (!validConfig)
	{
		logw("!invalid configuration!");
		_mqttServer[0] = '\0';
		_mqttPort[0] = '\0';
		_mqttUserName[0] = '\0';
		_mqttUserPassword[0] = '\0';
		_publishRateStr[0] = '\0';
		_iotWebConf.resetWifiAuthInfo();
	}
	else
	{
		_currentPublishRate = atoi(_publishRateStr) * 1000;
		_iotWebConf.skipApStartup(); // Set WIFI_AP_PIN to gnd to force AP mode
		if (_mqttServer[0] != '\0') // skip if no mqtt configured
		{
			logi("Valid configuration!");
			_MQTTConfigured = true;
			_mqttClient.onConnect(onMqttConnect);
			_mqttClient.onDisconnect(onMqttDisconnect);
			_mqttClient.onMessage(onMqttMessage);
			_mqttClient.onPublish(onMqttPublish);

			IPAddress ip;
			int port = atoi(_mqttPort);
			if (ip.fromString(_mqttServer))
			{
				_mqttClient.setServer(ip, port);
			}
			else
			{
				_mqttClient.setServer(_mqttServer, port);
			}
			_mqttClient.setCredentials(_mqttUserName, _mqttUserPassword);
		}
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
}

boolean IOT::Run()
{
	bool rVal = false;
	_iotWebConf.doLoop();
	if (WiFi.isConnected())
	{
		if (_MQTTConfigured) {
			rVal = _mqttClient.connected() && _NTPConfigured;
		}
		else {
			rVal = _NTPConfigured; // not using MQTT
		}
	}
	else
	{
		// set SSID/PW from flasher.exe app
		if (Serial.peek() == '{')
		{
			String s = Serial.readStringUntil('}');
			s += "}";
			JsonDocument doc;
			DeserializationError err = deserializeJson(doc, s);
			if (err)
			{
				loge("deserializeJson() failed: %s", err.c_str());
			}
			else
			{
				if (doc.containsKey("ssid") && doc.containsKey("password"))
				{
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
				else
				{
					logw("Received invalid json: %s", s.c_str());
				}
			}
		}
		else
		{
			Serial.read(); // discard data
		}
	}
	return rVal;
}

unsigned long IOT::PublishRate()
{
	return _currentPublishRate;
}

void IOT::SetPublishRate(unsigned long rate)
{
	_currentPublishRate = rate;
}

bool IOT::ProcessCmnd(char *payload, size_t len)
{
	return _cmdCB->handleCommand(payload, len);
}


void IOT::Publish(const char *subtopic, const char *value, boolean retained)
{
	if (_mqttClient.connected())
	{
		char buf[MQTT_TOPIC_LEN];
		sprintf(buf, "%s/stat/%s", _rootTopicPrefix, subtopic);
		_mqttClient.publish(buf, 0, retained, value);
	}
}

void IOT::Publish(const char *topic, float value, boolean retained)
{
	char buf[256];
	snprintf_P(buf, sizeof(buf), "%.1f", value);
	Publish(topic, buf, retained);
}

void IOT::PublishMessage(const char* topic, JsonDocument& payload) {
	String s;
	serializeJson(payload, s);
	if (_mqttClient.publish(topic, 0, false, s.c_str(), s.length()) == 0)
	{
		loge("**** Payload exceeds MAX MQTT Packet Size");
	}
}

void IOT::PublishTelemetery(bool online) {
	if (_lastTelemetery != online){
		_lastTelemetery = online;
		_mqttClient.publish(_willTopic, 0, true, online ? "Online": "Offline", 7);
	}
}

std::string IOT::getRootTopicPrefix() {
	std::string s(_rootTopicPrefix); 
	return s; 
};

std::string IOT::getThingName() {
	 std::string s(_iotWebConf.getThingName());
	  return s;
}

std::string IOT::getDeviceName() {
	 std::string s(_deviceName);
	  return s;
}

} // namespace EmailAlert