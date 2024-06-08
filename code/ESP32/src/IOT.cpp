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
AsyncMqttClient _mqttClient;
TimerHandle_t mqttReconnectTimer;
DNSServer _dnsServer;
WebServer _webServer(80);
HTTPUpdateServer _httpUpdater;
IotWebConf _iotWebConf(TAG, &_dnsServer, &_webServer, TAG, CONFIG_VERSION);

char _willTopic[MQTT_TOPIC_LEN];
char _rootTopicPrefix[MQTT_TOPIC_LEN];

iotwebconf::OptionalParameterGroup  MQTT_group = iotwebconf::OptionalParameterGroup ("MQTT", "MQTT", false);
iotwebconf::TextTParameter<CONFIG_LEN> mqttServerParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("mqttServer").label("MQTT server").defaultValue("").build();
iotwebconf::IntTParameter<uint16_t> mqttPortParam = iotwebconf::Builder<iotwebconf::IntTParameter<uint16_t>>("mqttSPort").label("MQTT port").defaultValue(1883).min(0).max(65535).build();
iotwebconf::TextTParameter<CONFIG_LEN> mqttUserNameParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("mqttUser").label("MQTT user").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> mqttUserPasswordParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("mqttPassword").label("MQTT password").defaultValue("").build();
iotwebconf::CheckboxTParameter setWillParam = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("setWill").label("Set Will").defaultValue(true).build();
iotwebconf::TextTParameter<STR_LEN> rootTopicParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("rootTopic").label("Root topic prefix").defaultValue(TAG).build();
iotwebconf::TextTParameter<CONFIG_LEN> statSubtopicParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("statSubtopicParam").label("Status subtopic").defaultValue("stat").build();
iotwebconf::TextTParameter<CONFIG_LEN> cmdSubtopicParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("cmdSubtopicParam").label("Command subtopic").defaultValue("cmd").build();
iotwebconf::TextTParameter<CONFIG_LEN> teleSubtopicParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("teleSubtopicParam").label("Telemetery subtopic").defaultValue("tele").build();

iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

void onMqttConnect(bool sessionPresent) {
	logd("Connected to MQTT. Session present: %d", sessionPresent);
	char buf[MQTT_TOPIC_LEN];
	int len = strlen(cmdSubtopicParam.value());
	if (len > 0){
		sprintf(buf, "%s/%s/#", _rootTopicPrefix, cmdSubtopicParam.value());
	} else {
		sprintf(buf, "%s/#", _rootTopicPrefix);
	}
	sprintf(buf, "%s/cmnd/#", _rootTopicPrefix);
	_mqttClient.subscribe(buf, 0);
	_mqttClient.publish(_willTopic, 0, true, "Online", 6);
	logi("Subscribed to [%s], qos: 0", buf);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
	logi("Disconnected from MQTT. Reason: %d", (int8_t)reason);
	if (WiFi.isConnected())
	{
		xTimerStart(mqttReconnectTimer, 0);
	}
	memset (_rootTopicPrefix, 0, MQTT_TOPIC_LEN);
}

void connectToMqtt() {
	if (WiFi.isConnected())
	{
		if (MQTT_group.isActive()) { // mqtt configured?
			logi("Connecting to MQTT...");
			int len = strlen(rootTopicParam.value());
			strncpy(_rootTopicPrefix, rootTopicParam.value(), len);
			if (strlen(teleSubtopicParam.value()) > 0){
				sprintf(_willTopic, "%s/%s", _rootTopicPrefix, teleSubtopicParam.value());
			} else {
				sprintf(_willTopic, "%s", _rootTopicPrefix);
			}
			if (setWillParam.value()){
				_mqttClient.setWill(_willTopic, 0, true, "Offline");
			}
			_mqttClient.connect();
			logd("rootTopicPrefix: %s", _rootTopicPrefix);
		}
	}
}

void WiFiEvent(WiFiEvent_t event) {
	logd("[WiFi-event] event: %d", event);
	String s;
	JsonDocument doc;
	switch (event)
	{
	case SYSTEM_EVENT_STA_GOT_IP:
		logd("Waiting for NTP server time reading");
		configTzTime(TZ, NTP_Server1, NTP_Server2);
		doc["IP"] = WiFi.localIP().toString();
		doc["ApPassword"] = TAG;
		serializeJson(doc, s);
		s += '\n';
		Serial.printf(s.c_str()); // send json to flash tool
		if (MQTT_group.isActive()) {// mqtt configured?
			xTimerStart(mqttReconnectTimer, 0); // connect to MQTT once we have wifi
		}
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		logi("WiFi lost connection");
		xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
		break;
	default:
		break;
	}
}

void onMqttPublish(uint16_t packetId) {
	logi("Publish acknowledged.  packetId: %d", packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
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
			// if (doc.containsKey("wakePublishRate") && doc["wakePublishRate"].is<int>())
			// {
			// 	int publishRate = doc["wakePublishRate"];
			// 	messageProcessed = true;
			// 	if (publishRate >= MIN_PUBLISH_RATE && publishRate <= MAX_PUBLISH_RATE)
			// 	{
			// 		_iot.SetPublishRate(doc["wakePublishRate"]);
			// 		logd("Wake publish rate: %d", _iot.PublishRate());
			// 	}
			// 	else
			// 	{
			// 		logd("wakePublishRate is out of rage!");
			// 	}
			// }
			if (!messageProcessed)
			{
				logd("MQTT Json payload {%s} not recognized!", payload);
			}
		}
	}
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
	if (MQTT_group.isActive()) {
		s += "MQTT:";
		s += "<ul>";
		s += htmlConfigEntry<char *>(mqttServerParam.label, mqttServerParam.value());
		s += htmlConfigEntry<uint16_t>(mqttPortParam.label, mqttPortParam.value());
		s += htmlConfigEntry<char *>(mqttUserNameParam.label, mqttUserNameParam.value());
		s += htmlConfigEntry<char *>(mqttUserPasswordParam.label, mqttUserPasswordParam.value());
		s += htmlConfigEntry<const char *>(setWillParam.label, setWillParam.value() ? "Enabled" : "Disabled");
		s += htmlConfigEntry<char *>(rootTopicParam.label, rootTopicParam.value());
		s += htmlConfigEntry<char *>(statSubtopicParam.label, statSubtopicParam.value());
		s += htmlConfigEntry<char *>(cmdSubtopicParam.label, cmdSubtopicParam.value());
		s += htmlConfigEntry<char *>(teleSubtopicParam.label, teleSubtopicParam.value());
		s += "</ul>";
	} else {
		s += "<ul>";
		s += "<li>MQTT not setup.";
		s += "</li></ul>";
	}
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
	if (MQTT_group.isActive()) {
		if ( requiredParam(webRequestWrapper, mqttServerParam) == false) return false;
		if ( requiredParam(webRequestWrapper, rootTopicParam) == false) return false;
		if ( requiredParam(webRequestWrapper, mqttPortParam) == false) return false;
	}
	return true;
}

void IOT::Init(IOTCallbackInterface* iotCB, MQTTCallbackInterface* cmdCB) {
	_iotCB = iotCB;
	_cmdCB = cmdCB;
	pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
	_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
	// setup EEPROM parameters
	MQTT_group.addItem(&mqttServerParam);
	MQTT_group.addItem(&mqttPortParam);
   	MQTT_group.addItem(&mqttUserNameParam);
	MQTT_group.addItem(&mqttUserPasswordParam);
	MQTT_group.addItem(&setWillParam);
	MQTT_group.addItem(&rootTopicParam);
	MQTT_group.addItem(&statSubtopicParam);
	MQTT_group.addItem(&cmdSubtopicParam);
	MQTT_group.addItem(&teleSubtopicParam);
	_iotWebConf.addParameterGroup(_iotCB->parameterGroup());
	_iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);
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
	logd("_iotWebConf.init complete");
	if (!validConfig)
	{
		logw("!invalid configuration!, set default values");
		// smtpServerParam.applyDefaultValue();
		// smtpPortParam.applyDefaultValue();
		// senderEmailParam.applyDefaultValue();
		// senderPasswordParam.applyDefaultValue();
		// recipientEmailParam.applyDefaultValue();
		// recipientNameParam.applyDefaultValue();
		MQTT_group.setActive(false);
		rootTopicParam.applyDefaultValue();
		mqttServerParam.applyDefaultValue();
		mqttPortParam.applyDefaultValue();
		mqttUserNameParam.applyDefaultValue();
		mqttUserPasswordParam.applyDefaultValue();
		_iotWebConf.resetWifiAuthInfo();
	}
	else
	{
		logi("Valid configuration!");
		_iotWebConf.skipApStartup();
		// _iotWebConf.goOffLine(); 
		#if TINY_GSM_MODEM_SIM7600
		logi("Setup GSM");
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
		gsm_modem.setNetworkMode(2); //Automatic
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
		String cop = gsm_modem.getOperator();
		IPAddress local = gsm_modem.localIP();
		logi("Operator: %s Local IP: %s", cop.c_str(), local.toString());
		#endif
		if (MQTT_group.isActive()) // skip if no mqtt configured
		{
			
			_MQTTConfigured = true;
			_mqttClient.onConnect(onMqttConnect);
			_mqttClient.onDisconnect(onMqttDisconnect);
			_mqttClient.onMessage(onMqttMessage);
			_mqttClient.onPublish(onMqttPublish);

			IPAddress ip;
			int port = mqttPortParam.value();
			if (ip.fromString(mqttServerParam.value()))
			{
				_mqttClient.setServer(ip, port);
			}
			else
			{
				_mqttClient.setServer(mqttServerParam.value(), port);
			}
			_mqttClient.setCredentials(mqttUserNameParam.value(), mqttUserPasswordParam.value());
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
	if (WiFi.isConnected()) {
		if (_NTPConfigured == false) {
			struct tm timeinfo;
			if(!getLocalTime(&timeinfo)){
				loge("Failed to obtain time 1");
				return rVal;
			}
			Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
			_NTPConfigured = true;
		} else if (_MQTTConfigured) {
			rVal = _mqttClient.connected();
		} else {
			rVal = _NTPConfigured; // not using MQTT
		}
	}
	else if (gsm_modem.isNetworkConnected()) {
		return true;
	}
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

bool IOT::ProcessCmnd(char *payload, size_t len) {
	return _cmdCB->handleCommand(payload, len);
}

void IOT::Publish(const char *subtopic, const char *value, boolean retained) {
	if (_mqttClient.connected()) {
		char buf[MQTT_TOPIC_LEN];
		int len = strlen(statSubtopicParam.value());
		if (len > 0){
			sprintf(buf, "%s/%s", _rootTopicPrefix, statSubtopicParam.value());
		} else {
			sprintf(buf, "%s", _rootTopicPrefix);
		}
		if (strlen(subtopic) > 0) {
			strcat(buf, "/");
			strcat(buf, subtopic);
		}
		logd("Publishing topic: %s payload: %s", buf, value);
		_mqttClient.publish(buf, 0, retained, value);
	} else {
		logw("MQTT client is not connected, unable to publish");
	}
}

void IOT::Publish(const char *topic, float value, boolean retained) {
	char buf[256];
	snprintf_P(buf, sizeof(buf), "%.1f", value);
	Publish(topic, buf, retained);
}

void IOT::Publish(const char* topic, JsonDocument& payload, boolean retained) {
	String s;
	serializeJson(payload, s);
	Publish(topic, s.c_str(), retained);
}

void IOT::PublishTelemetery(bool online) {
	if (_mqttClient.connected()) {
		if (_lastTelemetery != online){
			_lastTelemetery = online;
			_mqttClient.publish(_willTopic, 0, true, online ? "Online": "Offline", 7);
		}
	}
}

const char* IOT::getThingName() {
	return _iotWebConf.getThingName();
}

const char* IOT::getDeviceName() {
	return rootTopicParam.value();
}

void IOT::setGSMClient(SMTPSession* smtpSession) {
	if (!WiFi.isConnected()) {
		smtpSession->setGSMClient(&gsm_client, &gsm_modem, "", "", "", "");
	}
}
} // namespace SwitchNotifier