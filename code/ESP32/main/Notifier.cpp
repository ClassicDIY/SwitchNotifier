#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Notifier.h"
#include "HelperFunctions.h"
#include "IOT.h"
#include "Log.h"
#include "home.html"
#include "style.html"

// #include <LittleFS.h>

namespace CLASSICDIY {

static AsyncWebServer _asyncServer(ASYNC_WEBSERVER_PORT);
static AsyncWebSocket _webSocket("/ws_home");
IOT _iot = IOT();
static const char smtpPorts[][CONFIG_LEN] = {"25", "465", "587", "2525"};

Notifier::Notifier() { _smtp = new SMTPSession(); }

Notifier::~Notifier() { delete _smtp; }

void smtpCallback(SMTP_Status status) { logi("%s", status.info()); }

void Notifier::onSaveSetting(JsonDocument &doc) {
   JsonObject snmp = doc["snmp"].to<JsonObject>();
   snmp["smtpServer"] = _smtpServer;
   snmp["smtpPort"] = _smtpPort;
   snmp["senderEmail"] = _senderEmail;
   snmp["senderPassword"] = _senderPassword;
   snmp["recipientEmail"] = _recipientEmail;
   snmp["recipientName"] = _recipientName;
   snmp["subject"] = _subject;
   JsonObject messages = doc["messages"].to<JsonObject>();
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msg = "msg";
      msg += String(i);
      messages[msg.c_str()] = _buttons[i].message;
   }
}

void Notifier::onLoadSetting(JsonDocument &doc) {
   JsonObject snmp = doc["snmp"].as<JsonObject>();
   _smtpServer = snmp["smtpServer"].isNull() ? SMTP_server : snmp["smtpServer"].as<String>();
   _smtpPort = snmp["smtpPort"].isNull() ? SMTP_Port : snmp["smtpPort"].as<uint16_t>();
   _senderEmail = snmp["senderEmail"].isNull() ? "" : snmp["senderEmail"].as<String>();
   _senderPassword = snmp["senderPassword"].isNull() ? "" : snmp["senderPassword"].as<String>();
   _recipientEmail = snmp["recipientEmail"].isNull() ? "" : snmp["recipientEmail"].as<String>();
   _recipientName = snmp["recipientName"].isNull() ? "" : snmp["recipientName"].as<String>();
   _subject = snmp["subject"].isNull() ? "" : snmp["subject"].as<String>();
   JsonObject messages = doc["messages"].as<JsonObject>();
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msg = "msg";
      msg += String(i);
      _buttons[i].message = messages[msg.c_str()].as<std::string>();
   }
}

void Notifier::addApplicationConfigs(String &page) {
   String snmpFieldset = snmp_config_fieldset;
   String snmpFields;
   String snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "SMTP Server");
   snmpField.replace("{fld}", "smtpServer");
   snmpField.replace("{fldval}", _smtpServer.c_str());
   snmpFields += snmpField;
   snmpField = snmp_select;
   snmpField.replace("{fldlabel}", "SMTP Port");
   snmpField.replace("{fld}", "smtpPort");
   String options;
   for (int i = 0; i < sizeof(smtpPorts) / CONFIG_LEN; i++) {
      String option = snmp_select_option;
      option.replace("{fldval}", smtpPorts[i]);
      option.replace("{selected}", String(_smtpPort) == smtpPorts[i] ? "selected" : "");
      options += option;
   }
   snmpField.replace("{options}", options);
   snmpFields += snmpField;
   snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "Sender Email");
   snmpField.replace("{fld}", "senderEmail");
   snmpField.replace("{fldval}", _senderEmail.c_str());
   snmpFields += snmpField;
   snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "Sender Password");
   snmpField.replace("{fld}", "senderPassword");
   snmpField.replace("{fldval}", _senderPassword.c_str());
   snmpFields += snmpField;
   snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "Recipient Email");
   snmpField.replace("{fld}", "recipientEmail");
   snmpField.replace("{fldval}", _recipientEmail.c_str());
   snmpFields += snmpField;
   snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "Recipient Name");
   snmpField.replace("{fld}", "recipientName");
   snmpField.replace("{fldval}", _recipientName.c_str());
   snmpFields += snmpField;
   snmpField = snmp_input;
   snmpField.replace("{fldlabel}", "Subject");
   snmpField.replace("{fld}", "subject");
   snmpField.replace("{fldval}", _subject.c_str());
   snmpFields += snmpField;
   snmpFieldset.replace("{snmp}", snmpFields);
   page += snmpFieldset;
   String msgFields = app_config_fieldset;
   String msg_flds;
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msg_fld = app_input;
      msg_fld.replace("{bm}", String(i));
      msg_fld.replace("{message}", _buttons[i].message.c_str());
      msg_flds += msg_fld;
   }
   msgFields.replace("{aconv}", msg_flds);
   page += msgFields;
}

void Notifier::onSubmitForm(AsyncWebServerRequest *request) {
   if (request->hasParam("smtpServer", true)) {
      _smtpServer = request->getParam("smtpServer", true)->value().c_str();
   }
   if (request->hasParam("smtpPort", true)) {
      _smtpPort = request->getParam("smtpPort", true)->value().toInt();
   }
   if (request->hasParam("senderEmail", true)) {
      _senderEmail = request->getParam("senderEmail", true)->value().c_str();
   }
   if (request->hasParam("senderPassword", true)) {
      _senderPassword = request->getParam("senderPassword", true)->value().c_str();
   }
   if (request->hasParam("recipientEmail", true)) {
      _recipientEmail = request->getParam("recipientEmail", true)->value().c_str();
   }
   if (request->hasParam("recipientName", true)) {
      _recipientName = request->getParam("recipientName", true)->value().c_str();
   }
   if (request->hasParam("subject", true)) {
      _subject = request->getParam("subject", true)->value().c_str();
   }
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msgn = "message";
      msgn += String(i);
      if (request->hasParam(msgn, true)) {
         _buttons[i].message = request->getParam(msgn, true)->value().c_str();
      }
   }
}

void Notifier::setup() {
   logd("setup");
   _iot.Init(this, &_asyncServer);
   _smtp->callback(smtpCallback);
   _smtp->setTCPTimeout(10);
   gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
   for (int i = 0; i < NUM_BUTTONS; ++i) {
      _buttons[i].init();
   }
   _asyncServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      String page = home_html;
      page.replace("{style}", style);
      page.replace("{n}", _iot.getThingName().c_str());
      page.replace("{v}", APP_VERSION);
      std::string s;
      s += "<div class='desc' >SMTP Server: ";
      s += _smtpServer.c_str();
      s += "</div> <div class='desc' >SMTP Port: ";
      s += std::to_string(_smtpPort);
      s += "</div> <div class='desc' >Sender Email: ";
      s += _senderEmail.c_str();
      s += "</div> <div class='desc' >Recipient Email: ";
      s += _recipientEmail.c_str();
      s += "</div> <div class='desc' >Recipient Name: ";
      s += _recipientName.c_str();
      s += "</div> <div class='desc' >Subject: ";
      s += _subject.c_str();
      s += "</div>";
      page.replace("{smtpSettings}", s.c_str());
      s.clear();
      for (int i = 0; i < NUM_BUTTONS; i++) {
         s += "<div class='desc' >";
         s += "DI";
         s += std::to_string(i);
         s += ": ";
         s += _buttons[i].message.c_str();
         s += "</div>";
      }
      page.replace("{emailMessages}", s.c_str());

      request->send(200, "text/html", page);
   });
   _asyncServer.addHandler(&_webSocket).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next) {
      // ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
      if (_webSocket.count() > 1) {
         // if we have 2 clients or more, prevent the next one to connect
         request->send(503, "text/plain", "Server is busy");
      } else {
         // process next middleware and at the end the handler
         next();
      }
   });
   _webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
      (void)len;
      if (type == WS_EVT_CONNECT) {
         // _lastMessagePublished.clear(); //force a broadcast
         client->setCloseClientOnQueueFull(false);
         client->ping();
      } else if (type == WS_EVT_DISCONNECT) {
         // logi("Home Page Disconnected!");
      } else if (type == WS_EVT_ERROR) {
         loge("ws error");
         // } else if (type == WS_EVT_PONG) {
         // 	logd("ws pong");
      }
   });
//    if (!LittleFS.begin()) {
//     loge("LittleFS mount failed");
//   }

}

void Notifier::run() {
   _iot.Run();
   for (int i = 0; i < NUM_BUTTONS; i++) {
      if (_buttons[i].pressed) {
         _buttons[i].pressed = false;
         logd("sendit: %s", _buttons[i].message.c_str());
         sendit(String(_buttons[i].message.c_str()));
      }
      delay(5);
   }
}

void Notifier::sendit(const String &content) {

   logd("SMTP: %s:%d From:%s PW: %s To: (%s)%s", _smtpServer.c_str(), _smtpPort, _senderEmail.c_str(), _senderPassword.c_str(), _recipientName.c_str(), _recipientEmail.c_str());
   Session_Config session;
   session.server.host_name = _smtpServer.c_str();

   session.server.port = _smtpPort;
   session.login.email = _senderEmail.c_str();
   session.login.password = _senderPassword.c_str();
   session.login.user_domain = "";
   /* Declare the message class */
   SMTP_Message message;
   message.sender.name = "ESP 32";
   message.sender.email = _senderEmail.c_str();
   message.subject = _subject.c_str();
   message.addRecipient(_recipientName.c_str(), _recipientEmail.c_str());
   message.timestamp.tag = "#esp_mail_current_time";
   message.timestamp.format = "%B %d, %Y %H:%M:%S";
   // Send HTML message
   String htmlMsg = "<div style=\"color:#000000;\"><h1>";
   htmlMsg.concat(content.c_str());
   htmlMsg.concat("</h1><p> Mail Generated from SwitchNotifier</p></div>");
   message.html.content = htmlMsg.c_str();
   message.text.charSet = "us-ascii";
   message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
   // /* //Send simple text message
   // String textMsg = "How are you doing";
   // message.text.content = textMsg.c_str();
   // message.text.charSet = "us-ascii";
   // message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

   if (!_smtp->connect(&session, true)) {
      loge("SMTP not connected");
      return;
   }
   if (!_smtp->isLoggedIn()) {
      logw("SMTP not yet logged in.");
   } else {
      if (_smtp->isAuthenticated()) {
         logi("SMTP successfully logged in.");
      } else {
         logw("Connected with no Auth.");
      }
      if (!MailClient.sendMail(_smtp, &message)) {
         loge("SMTP Error, Status Code: %d, Error Code: %d, Reason: %s", _smtp->statusCode(), _smtp->errorCode(), _smtp->errorReason());
      }
   }
}

} // namespace CLASSICDIY