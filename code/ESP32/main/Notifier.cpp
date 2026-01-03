#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Notifier.h"
#include "HelperFunctions.h"
#include "IOT.h"
#include "Log.h"
#include "app.htm"
#include "app_script.js"

IOT _iot = IOT();

Notifier::Notifier() { _smtp = new SMTPSession(); }

Notifier::~Notifier() { delete _smtp; }

void smtpCallback(SMTP_Status status) { logi("%s", status.info()); }

void Notifier::setup() {
   Init();
   _iot.Init(this);
   _smtp->callback(smtpCallback);
   _smtp->setTCPTimeout(10);
   gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
   for (int i = 0; i < NUM_BUTTONS; ++i) {
      _buttons[i].init();
   }
}

void Notifier::onSaveSetting(JsonDocument &doc) {
   doc["smtpServer"] = _smtpServer;
   doc["smtpPort"] = _smtpPort;
   doc["senderEmail"] = _senderEmail;
   doc["senderPassword"] = _senderPassword;
   doc["recipientEmail"] = _recipientEmail;
   doc["recipientName"] = _recipientName;
   doc["subject"] = _subject;
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msg = "msg";
      msg += String(i);
      doc[msg.c_str()] = _buttons[i].message;
   }
}

void Notifier::onLoadSetting(JsonDocument &doc) {
   _smtpServer = doc["smtpServer"].isNull() ? SMTP_server : doc["smtpServer"].as<String>();
   _smtpPort = doc["smtpPort"].isNull() ? SMTP_Port : doc["smtpPort"].as<uint16_t>();
   _senderEmail = doc["senderEmail"].isNull() ? "" : doc["senderEmail"].as<String>();
   _senderPassword = doc["senderPassword"].isNull() ? "" : doc["senderPassword"].as<String>();
   _recipientEmail = doc["recipientEmail"].isNull() ? "" : doc["recipientEmail"].as<String>();
   _recipientName = doc["recipientName"].isNull() ? "" : doc["recipientName"].as<String>();
   _subject = doc["subject"].isNull() ? "" : doc["subject"].as<String>();
   for (int i = 0; i < NUM_BUTTONS; i++) {
      String msg = "msg";
      msg += String(i);
      _buttons[i].message = doc[msg.c_str()].as<std::string>();
   }
}

String Notifier::appTemplateProcessor(const String &var) {
   if (var == "title") {
      return String(_iot.getThingName().c_str());
   }
   if (var == "version") {
      return String(APP_VERSION);
   }
   if (var == "home_html") {
      return String(home_html);
   }
   if (var == "emailMessages") {
      String s;
      for (int i = 0; i < NUM_BUTTONS; i++) {
         s += "<div class='desc' >";
         s += "DI";
         s += String(i);
         s += ": ";
         s += _buttons[i].message.c_str();
         s += "</div>";
      }
      return s;
   }
   if (var == "app_fields") {
      return String(snmp_config_fieldset);
   }
   if (var == "aconv") {
      String msg_flds;
      for (int i = 0; i < NUM_BUTTONS; i++) {
         String msg_fld = app_input;
         msg_fld.replace("{bm}", String(i));
         msg_flds += msg_fld;
      }
      return String(msg_flds);
   }
   if (var == "app_script_js") {
      return String(app_script_js);
   }
   logd("Did not find app template for: %s", var.c_str());
   return String("");
}

void Notifier::onSocketPong() {
   // _lastMessagePublished.clear(); // force a broadcast
}

void Notifier::onNetworkState(NetworkState state) {
   _networkState = state;
   if (state == OnLine) {
      configTime(0, 0, NTP_SERVER, NTP_SERVER2);
      setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);
      tzset();
   }
};
void Notifier::run() {
   _iot.Run();
   Run(); // base class
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
