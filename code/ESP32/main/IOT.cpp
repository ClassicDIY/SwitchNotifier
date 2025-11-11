#include "IOT.h"
#include "HelperFunctions.h"
#include "IOT.html"
#include "Log.h"
#include "WebLog.h"
#include "driver/spi_master.h"
#include "esp_mac.h"
#include "style.html"
#include <ESPmDNS.h>
#include <SPI.h>
#include <chrono>
#include <esp_event.h>
#include <sys/time.h>
#include <thread>

#ifdef Has_OLED_Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 oled_display;

#endif

namespace CLASSICDIY {
TimerHandle_t mqttReconnectTimer;
static DNSServer _dnsServer;
static WebLog _webLog;
static AsyncAuthenticationMiddleware basicAuth;

// #pragma region Setup
void IOT::Init(IOTCallbackInterface *iotCB, AsyncWebServer *pwebServer) {
    _iotCB = iotCB;
    _pwebServer = pwebServer;
#ifdef WIFI_STATUS_PIN
    pinMode(WIFI_STATUS_PIN,
            OUTPUT); // use LED for wifi AP status (note:edgeBox shares the LED pin with the serial TX gpio)
#endif
#ifdef FACTORY_RESET_PIN // use digital input pin for factory reset
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    EEPROM.begin(EEPROM_SIZE);
    if (digitalRead(FACTORY_RESET_PIN) == LOW) {
        logi("Factory Reset");
        EEPROM.write(0, 0);
        EEPROM.commit();
    }
#else // use analog pin for factory reset
    EEPROM.begin(EEPROM_SIZE);
    uint16_t analogValue = analogRead(BUTTONS);
    logd("button value (%d)", analogValue);
    if (analogValue > 3000) {
        logi("**********************Factory Reset*************************(%d)", analogValue);
        EEPROM.write(0, 0);
        EEPROM.commit();
        saveSettings();
    }
#endif
    else {
        loadSettings();
    }
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        String s;
        JsonDocument doc;
        switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            logd("AP_STADISCONNECTED");
            _AP_Connected = false;
            GoOffline();
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            logd("AP_STAIPASSIGNED");
            sprintf(_Current_IP, "%s", WiFi.softAPIP().toString().c_str());
            logd("Current_IP: %s", _Current_IP);
            _AP_Connected = true;
            GoOnline();
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            logd("STA_GOT_IP");
            doc["IP"] = WiFi.localIP().toString().c_str();
            sprintf(_Current_IP, "%s", WiFi.localIP().toString().c_str());
            logi("Got IP Address");
            logi("~~~~~~~~~~~");
            logi("IP: %s", _Current_IP);
            logi("IPMASK: %s", WiFi.subnetMask().toString().c_str());
            logi("Gateway: %s", WiFi.gatewayIP().toString().c_str());
            logi("~~~~~~~~~~~");
            doc["ApPassword"] = DEFAULT_AP_PASSWORD;
            serializeJson(doc, s);
            s += '\n';
            Serial.printf(s.c_str()); // send json to flash tool
            configTime(0, 0, NTP_SERVER);
            printLocalTime();
            GoOnline();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            logw("STA_DISCONNECTED");
            GoOffline();
            break;
        default:
            logd("[WiFi-event] event: %d", event);
            break;
        }
    });
    // generate unique id from mac address NIC segment
    uint8_t chipid[6];
    esp_efuse_mac_get_default(chipid);
    _uniqueId = chipid[3] << 16;
    _uniqueId += chipid[4] << 8;
    _uniqueId += chipid[5];
    _lastBootTimeStamp = millis();
    _pwebServer->on("/reboot", [this](AsyncWebServerRequest *request) {
        RedirectToHome(request);
        _needToReboot = true;
    });
    _pwebServer->onNotFound([this](AsyncWebServerRequest *request) { RedirectToHome(request); });
    basicAuth.setUsername("admin");
    basicAuth.setPassword(_AP_Password.c_str());
    basicAuth.setAuthFailureMessage("Authentication failed");
    basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
    basicAuth.generateHash();
    _pwebServer
        ->on("/settings", HTTP_GET,
             [this](AsyncWebServerRequest *request) {
                 String fields = network_config_fields;
                 fields.replace("{n}", _AP_SSID);
                 fields.replace("{v}", APP_VERSION);
                 fields.replace("{AP_SSID}", _AP_SSID);
                 fields.replace("{AP_Pw}", _AP_Password);
                 fields.replace("{WIFI}", _NetworkSelection == WiFiMode ? "selected" : "");
#ifdef HasEthernet
                 fields.replace("{ETH}", _NetworkSelection == EthernetMode ? "selected" : "");
#else
			fields.replace("{ETH}", "class='hidden'");
#endif
                 fields.replace("{4G}", _NetworkSelection == ModemMode ? "selected" : "");
                 fields.replace("{SSID}", _SSID);
                 fields.replace("{WiFi_Pw}", _WiFi_Password);
                 fields.replace("{dhcpChecked}", _useDHCP ? "checked" : "unchecked");
                 fields.replace("{APN}", _APN);
                 fields.replace("{SIM_USERNAME}", _SIM_Username);
                 fields.replace("{SIM_PASSWORD}", _SIM_Password);
                 fields.replace("{SIM_PIN}", _SIM_PIN);
                 String page = network_config_top;
                 page.replace("{style}", style);
                 page.replace("{n}", _AP_SSID);
                 page.replace("{v}", APP_VERSION);
                 page += fields;
                 _iotCB->addApplicationConfigs(page);
                 String apply_button = network_config_apply_button;
                 page += apply_button;
#ifdef HasOTA
                 page += network_config_links;
#else 
                page += network_config_links_no_ota;
#endif
                 request->send(200, "text/html", page);
             })
        .addMiddleware(&basicAuth);

    _pwebServer->on("/submit", HTTP_POST, [this](AsyncWebServerRequest *request) {
        logd("submit");
        if (request->hasParam("AP_SSID", true)) {
            _AP_SSID = request->getParam("AP_SSID", true)->value().c_str();
        }
        if (request->hasParam("AP_Pw", true)) {
            _AP_Password = request->getParam("AP_Pw", true)->value().c_str();
        }
        if (request->hasParam("SSID", true)) {
            _SSID = request->getParam("SSID", true)->value().c_str();
        }
        if (request->hasParam("networkSelector", true)) {
            String sel = request->getParam("networkSelector", true)->value();
            _NetworkSelection = sel == "wifi" ? WiFiMode : sel == "ethernet" ? EthernetMode : ModemMode;
        }
        if (request->hasParam("WiFi_Pw", true)) {
            _WiFi_Password = request->getParam("WiFi_Pw", true)->value().c_str();
        }
        if (request->hasParam("APN", true)) {
            _APN = request->getParam("APN", true)->value().c_str();
        }
        if (request->hasParam("SIM_USERNAME", true)) {
            _SIM_Username = request->getParam("SIM_USERNAME", true)->value().c_str();
        }
        if (request->hasParam("SIM_PASSWORD", true)) {
            _SIM_Password = request->getParam("SIM_PASSWORD", true)->value().c_str();
        }
        if (request->hasParam("SIM_PIN", true)) {
            _SIM_PIN = request->getParam("SIM_PIN", true)->value().c_str();
        }
        _iotCB->onSubmitForm(request);
        RedirectToHome(request);
        saveSettings();
    });
}

void IOT::RedirectToHome(AsyncWebServerRequest *request) {
    logd("Redirecting from: %s", request->url().c_str());
    String page = redirect_html;
    page.replace("{n}", _SSID);
    page.replace("{ip}", _Current_IP);
    request->send(200, "text/html", page);
}

void IOT::loadSettings() {
    String jsonString;
    char ch;
    for (int i = 0; i < EEPROM_SIZE; ++i) {
        ch = EEPROM.read(i);
        if (ch == '\0')
            break; // Stop at the null terminator
        jsonString += ch;
        _settingsChecksum += ch;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        loge("Failed to load data from EEPROM, using defaults: %s", error.c_str());
        saveSettings(); // save default values
    } else {
        logd("JSON loaded from EEPROM: %d", jsonString.length());
        printFormattedJson(doc);
        JsonObject iot = doc["iot"].as<JsonObject>();
        _AP_SSID = iot["AP_SSID"].isNull() ? TAG : iot["AP_SSID"].as<String>();
        _AP_Password = iot["AP_Pw"].isNull() ? DEFAULT_AP_PASSWORD : iot["AP_Pw"].as<String>();
        _NetworkSelection = iot["Network"].isNull() ? WiFiMode : iot["Network"].as<NetworkSelection>();
        _SSID = iot["SSID"].isNull() ? "" : iot["SSID"].as<String>();
        _WiFi_Password = iot["WiFi_Pw"].isNull() ? "" : iot["WiFi_Pw"].as<String>();
        _APN = iot["APN"].isNull() ? "" : iot["APN"].as<String>();
        _SIM_Username = iot["SIM_USERNAME"].isNull() ? "" : iot["SIM_USERNAME"].as<String>();
        _SIM_Password = iot["SIM_PASSWORD"].isNull() ? "" : iot["SIM_PASSWORD"].as<String>();
        _SIM_PIN = iot["SIM_PIN"].isNull() ? "" : iot["SIM_PIN"].as<String>();
        _useDHCP = iot["useDHCP"].isNull() ? false : iot["useDHCP"].as<bool>();
        _iotCB->onLoadSetting(doc);
    }
}

void IOT::saveSettings() {
    JsonDocument doc;
    JsonObject iot = doc["iot"].to<JsonObject>();
    iot["version"] = APP_VERSION;
    iot["AP_SSID"] = _AP_SSID;
    iot["AP_Pw"] = _AP_Password;
    iot["Network"] = _NetworkSelection;
    iot["SSID"] = _SSID;
    iot["WiFi_Pw"] = _WiFi_Password;
    iot["APN"] = _APN;
    iot["SIM_USERNAME"] = _SIM_Username;
    iot["SIM_PASSWORD"] = _SIM_Password;
    iot["SIM_PIN"] = _SIM_PIN;
    iot["useDHCP"] = _useDHCP;
    _iotCB->onSaveSetting(doc);
    String jsonString;
    serializeJson(doc, jsonString);
    printFormattedJson(doc);
    uint32_t sum = 0;
    for (int i = 0; i < jsonString.length(); ++i) {
        int8_t byte = jsonString[i];
        EEPROM.write(i, byte);
        sum += byte;
    }
    EEPROM.write(jsonString.length(), '\0'); // Null-terminate the string
    EEPROM.commit();
    logd("JSON saved, required EEPROM size: %d", jsonString.length());
    _needToReboot = _settingsChecksum != sum;
    if (_needToReboot)
        logd("******* Need to reboot! ***");
}

void IOT::Run() {
    uint32_t now = millis();
    if (_networkState == Boot &&
        _NetworkSelection == NotConnected) { // Network not setup?, see if flasher is trying to send us the SSID/Pw
        if (Serial.peek() == '{') {
            String s = Serial.readStringUntil('}');
            s += "}";
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, s);
            if (err) {
                loge("deserializeJson() failed: %s", err.c_str());
            } else {
                if (doc["ssid"].is<const char *>() && doc["password"].is<const char *>()) {
                    _SSID = doc["ssid"].as<String>();
                    logd("Setting ssid: %s", _SSID.c_str());
                    _WiFi_Password = doc["password"].as<String>();
                    logd("Setting password: %s", _WiFi_Password.c_str());
                    _NetworkSelection = WiFiMode;
                    saveSettings();
                    esp_restart();
                } else {
                    logw("Received invalid json: %s", s.c_str());
                }
            }
        } else {
            Serial.read(); // discard data
        }
        if ((now - _FlasherIPConfigStart) > FLASHER_TIMEOUT) // wait for flasher tool to send Wifi info
        {
            logd("Done waiting for flasher!");
            setState(ApState); // switch to AP mode for AP_TIMEOUT
        }
    } else if (_networkState == Boot) // have network selection, start with wifiAP for AP_TIMEOUT then STA mode
    {
        setState(ApState); // switch to AP mode for AP_TIMEOUT
    } else if (_networkState == ApState) {
        if (_AP_Connected == false) // if AP client is connected, stay in AP mode
        {
            if ((now - _waitInAPTimeStamp) >
                AP_TIMEOUT) // switch to selected network after waiting in APMode for AP_TIMEOUT duration
            {
                if (_SSID.length() > 0) // is it setup yet?
                {
                    logd("Connecting to network: %d", _NetworkSelection);
                    setState(Connecting);
                }
            } else {
                UpdateOledDisplay(); // update countdown
            }
        }
        _dnsServer.processNextRequest();
        _webLog.process();
    } else if (_networkState == Connecting) {
        if ((millis() - _NetworkConnectionStart) > WIFI_CONNECTION_TIMEOUT) {
            // -- Network not available, fall back to AP mode.
            logw("Giving up on Network connection.");
            WiFi.disconnect();
            setState(ApState);
        }
    } else if (_networkState == OffLine) // went offline, try again...
    {
        logw("went offline, try again...");
        setState(Connecting);
    } else if (_networkState == OnLine) {
        _webLog.process();
    }
#ifdef WIFI_STATUS_PIN
    // use LED if the log level is none (edgeBox shares the LED pin with the serial TX gpio)
    // handle blink led, fast : NotConnected slow: AP connected On: Station connected
    if (_networkState != OnLine) {
        unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
        unsigned long now = millis();
        if (binkRate < now - _lastBlinkTime) {
            _blinkStateOn = !_blinkStateOn;
            _lastBlinkTime = now;
            digitalWrite(WIFI_STATUS_PIN, _blinkStateOn ? HIGH : LOW);
        }
    } else {
        digitalWrite(WIFI_STATUS_PIN, HIGH);
    }
#elif RGB_LED_PIN
    if (_networkState != OnLine) {
        unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
        unsigned long now = millis();
        if (binkRate < now - _lastBlinkTime) {
            _blinkStateOn = !_blinkStateOn;
            _lastBlinkTime = now;
            RGB_Light(_blinkStateOn ? 60 : 0, _blinkStateOn ? 0 : 60, 0);
        }
    } else {
        RGB_Light(0, 0, 60);
    }
#endif

    if (_needToReboot) {
        GoOffline();
        delay(500);
        esp_restart();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    return;
}

void IOT::UpdateOledDisplay() {
#ifdef Has_OLED_Display
    oled_display.clearDisplay();
    oled_display.setTextSize(2);
    oled_display.setTextColor(SSD1306_WHITE);
    oled_display.setCursor(0, 0);
    oled_display.println("ESP_PLC");
    oled_display.setTextSize(1);
    oled_display.println(APP_VERSION);
    oled_display.setTextSize(2);
    oled_display.setCursor(0, 30);

    if (_networkState == OnLine) {
        oled_display.println(_NetworkSelection == WiFiMode ? "WiFi: " : "LTE: ");
        oled_display.setTextSize(1);
        oled_display.println(_Current_IP);
    } else if (_networkState == Connecting) {
        oled_display.println("Connecting...");
    } else if (_networkState == ApState) {
        oled_display.println("AP Mode");
        int countdown = (AP_TIMEOUT - (millis() - _waitInAPTimeStamp)) / 1000;
        if (countdown > 0) {
            oled_display.setTextSize(2);
            oled_display.printf("%d", countdown);
        }
    } else {
        oled_display.println("Offline");
    }
    oled_display.display();
#endif
}

// #pragma endregion Setup

// #pragma region Network

void IOT::GoOnline() {
    logd("GoOnline called");
    _pwebServer->begin();
    _webLog.begin(_pwebServer);
#ifdef HasOTA
    _OTA.begin(_pwebServer);
#endif
    if (_networkState > ApState) {
        if (_NetworkSelection == EthernetMode || _NetworkSelection == WiFiMode) {
            MDNS.begin(_AP_SSID.c_str());
            MDNS.addService("http", "tcp", ASYNC_WEBSERVER_PORT);
            logd("Active mDNS services: %d", MDNS.queryService("http", "tcp"));
        }
        _iotCB->onNetworkConnect();
        setState(OnLine);
    }
}

void IOT::GoOffline() {
    logd("GoOffline");
    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    _webLog.end();
    _dnsServer.stop();
    MDNS.end();
    if (_networkState == OnLine) {
        setState(OffLine);
    }
}

void IOT::setState(NetworkState newState) {
    NetworkState oldState = _networkState;
    _networkState = newState;
    logd("_networkState: %s", _networkState == Boot         ? "Boot"
                              : _networkState == ApState    ? "ApState"
                              : _networkState == Connecting ? "Connecting"
                              : _networkState == OnLine     ? "OnLine"
                                                            : "OffLine");
    UpdateOledDisplay();
    switch (newState) {
    case OffLine:
        if (_NetworkSelection == WiFiMode) {
            WiFi.disconnect(true); // true = erase WiFi credentials
            WiFi.mode(WIFI_OFF);   // disable Wi-Fi hardware
        } else if (_NetworkSelection == EthernetMode) {
            DisconnectEthernet();
        }
        delay(100);
        break;
    case ApState:
        if ((oldState == Connecting) || (oldState == OnLine)) {
            if (_NetworkSelection == WiFiMode) {
                WiFi.disconnect(true); // true = erase WiFi credentials
            } else if (_NetworkSelection == EthernetMode) {
                DisconnectEthernet();
            }
            delay(100);
        }
        WiFi.mode(WIFI_AP);
        if (WiFi.softAP(_AP_SSID, _AP_Password)) {
            IPAddress IP = WiFi.softAPIP();
            logi("WiFi AP SSID: %s PW: %s", _AP_SSID.c_str(), _AP_Password.c_str());
            logd("AP IP address: %s", IP.toString().c_str());
            _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
            _dnsServer.start(DNS_PORT, "*", IP);
        }
        _waitInAPTimeStamp = millis();
        break;
    case Connecting:
        _NetworkConnectionStart = millis();
        if (_NetworkSelection == WiFiMode) {
            logd("WiFiMode, trying to connect to %s", _SSID.c_str());
            WiFi.setHostname(_AP_SSID.c_str());
            WiFi.mode(WIFI_STA);
            WiFi.begin(_SSID, _WiFi_Password);
        } else if (_NetworkSelection == EthernetMode) {
            if (ConnectEthernet() == ESP_OK) {
                logd("Ethernet succeeded");
            } else {
                loge("Failed to connect to Ethernet");
            }
        }
        break;
    case OnLine:
        logd("State: Online");
        break;
    default:
        break;
    }
}

#ifdef HasEthernet
void IOT::HandleIPEvent(int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (event_id == IP_EVENT_PPP_GOT_IP || event_id == IP_EVENT_ETH_GOT_IP) {
        const esp_netif_ip_info_t *ip_info = &event->ip_info;
        logi("Got IP Address");
        logi("~~~~~~~~~~~");
        logi("IP:" IPSTR, IP2STR(&ip_info->ip));
        sprintf(_Current_IP, IPSTR, IP2STR(&ip_info->ip));
        logi("IPMASK:" IPSTR, IP2STR(&ip_info->netmask));
        logi("Gateway:" IPSTR, IP2STR(&ip_info->gw));
        logi("~~~~~~~~~~~");
        GoOnline();
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        logi("Modem Disconnect from PPP Server");
        GoOffline();
    } else if (event_id == IP_EVENT_ETH_LOST_IP) {
        logi("Ethernet Disconnect");
        GoOffline();
    } else if (event_id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        logi("Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    } else {
        logd("IP event! %d", (int)event_id);
    }
}
#endif

esp_err_t IOT::ConnectEthernet() {
#ifdef HasEthernet
    esp_err_t ret = ESP_OK;
    logd("ConnectEthernet");
    if ((ret = gpio_install_isr_service(0)) != ESP_OK) {
        if (ret == ESP_ERR_INVALID_STATE) {
            logw("GPIO ISR handler has been already installed");
            ret = ESP_OK; // ISR handler has been already installed so no issues
        } else {
            logd("GPIO ISR handler install failed");
        }
    }
    spi_bus_config_t buscfg = {
        .mosi_io_num = ETH_MOSI,
        .miso_io_num = ETH_MISO,
        .sclk_io_num = ETH_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    if ((ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO)) != ESP_OK) {
        logd("SPI host #1 init failed");
        return ret;
    }
    uint8_t base_mac_addr[6];
    if ((ret = esp_efuse_mac_get_default(base_mac_addr)) == ESP_OK) {
        uint8_t local_mac_1[6];
        esp_derive_local_mac(local_mac_1, base_mac_addr);
        logi("ETH MAC: %02X:%02X:%02X:%02X:%02X:%02X", local_mac_1[0], local_mac_1[1], local_mac_1[2], local_mac_1[3],
             local_mac_1[4], local_mac_1[5]);
        eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG(); // Init common MAC and PHY configs to default
        eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
        phy_config.phy_addr = 1;
        phy_config.reset_gpio_num = ETH_RST;
        spi_device_interface_config_t spi_devcfg = {
            .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
            .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
            .mode = 0,
            .clock_speed_hz = 25 * 1000 * 1000,
            .spics_io_num = ETH_SS,
            .queue_size = 20,
        };
        spi_device_handle_t spi_handle;
        if ((ret = spi_bus_add_device(SPI2_HOST, &spi_devcfg, &spi_handle)) != ESP_OK) {
            loge("spi_bus_add_device failed");
            return ret;
        }
        eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
        w5500_config.int_gpio_num = ETH_INT;
        esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
        esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
        _eth_handle = NULL;
        esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
        if ((ret = esp_eth_driver_install(&eth_config_spi, &_eth_handle)) != ESP_OK) {
            loge("esp_eth_driver_install failed");
            return ret;
        }
        if ((ret = esp_eth_ioctl(_eth_handle, ETH_CMD_S_MAC_ADDR, local_mac_1)) != ESP_OK) // set mac address
        {
            logd("SPI Ethernet MAC address config failed");
        }
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH(); // Initialize the Ethernet interface
        _netif = esp_netif_new(&cfg);
        assert(_netif);
        if (!_useDHCP) {
            esp_netif_dhcpc_stop(_netif);
            esp_netif_ip_info_t ipInfo;
            IPAddress ip;
            ip.fromString(_Static_IP);
            ipInfo.ip.addr = static_cast<uint32_t>(ip);
            ip.fromString(_Subnet_Mask);
            ipInfo.netmask.addr = static_cast<uint32_t>(ip);
            ip.fromString(_Gateway_IP);
            ipInfo.gw.addr = static_cast<uint32_t>(ip);
            if ((ret = esp_netif_set_ip_info(_netif, &ipInfo)) != ESP_OK) {
                loge("esp_netif_set_ip_info failed: %d", ret);
                return ret;
            }
        }
        _eth_netif_glue = esp_eth_new_netif_glue(_eth_handle);
        if ((ret = esp_netif_attach(_netif, _eth_netif_glue)) != ESP_OK) {
            loge("esp_netif_attach failed");
            return ret;
        }
        if ((ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, this)) != ESP_OK) {
            loge("esp_event_handler_register IP_EVENT->IP_EVENT_ETH_GOT_IP failed");
            return ret;
        }
        if ((ret = esp_eth_start(_eth_handle)) != ESP_OK) {
            loge("esp_netif_attach failed");
            return ret;
        }
    }
    return ret;
#else
    loge("Ethernet not supported on this device");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

void IOT::DisconnectEthernet() {
#ifdef HasEthernet
    if (_eth_handle != NULL) {
        ESP_ERROR_CHECK(esp_eth_stop(_eth_handle));
        _eth_handle = NULL;
        if (_eth_netif_glue != NULL) {
            ESP_ERROR_CHECK(esp_eth_del_netif_glue(_eth_netif_glue));
            _eth_netif_glue = NULL;
        }
        if (_netif != NULL) {
            esp_netif_destroy(_netif);
            _netif = NULL;
        }
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, on_ip_event));
    }
#endif
}

// #pragma endregion Network

std::string IOT::getThingName() {
    std::string s(_AP_SSID.c_str());
    return s;
}

#if TINY_GSM_MODEM_SIM7600
void IOT::setGSMClient(SMTPSession *smtpSession) {
    // if (useGSM.isChecked()) {
    // 	smtpSession->setGSMClient(&gsm_client, &gsm_modem, pinGPRSParam.value(), apnGPRSParam.value(),
    // userGPRSParam.value(), passGPRSParam.value());
    // }
}
#endif

} // namespace CLASSICDIY