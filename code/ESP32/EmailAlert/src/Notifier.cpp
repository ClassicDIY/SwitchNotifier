#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "InlineFunctions.h"
#include "Notifier.h"
#include "Log.h"

namespace SwitchNotifier
{
static const char smtpPorts[][CONFIG_LEN] = { "25", "465", "587", "2525" };
iotwebconf::ParameterGroup SMTP_group = iotwebconf::ParameterGroup("Email", "SMTP");
iotwebconf::TextTParameter<CONFIG_LEN> smtpServerParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("smtpServer").label("SMTP server").defaultValue(SMTP_server).build();
iotwebconf::SelectTParameter<CONFIG_LEN> smtpPortParam = iotwebconf::Builder<iotwebconf::SelectTParameter<CONFIG_LEN>>("smtpPort").label("SMTP port").
	optionValues((const char*)smtpPorts).optionNames((const char*)smtpPorts).optionCount(sizeof(smtpPorts) / CONFIG_LEN).nameLength(CONFIG_LEN).defaultValue("465").build();
iotwebconf::TextTParameter<CONFIG_LEN> senderEmailParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("senderEmail").label("Sender Email").defaultValue("").build();
iotwebconf::PasswordTParameter<CONFIG_LEN> senderPasswordParam = iotwebconf::Builder<iotwebconf::PasswordTParameter<CONFIG_LEN>>("senderPassword").label("Sender password").defaultValue("").build();
iotwebconf::TextTParameter<CONFIG_LEN> recipientEmailParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("recipientEmail").label("Recipient Email").defaultValue("").build();
iotwebconf::TextTParameter<CONFIG_LEN> recipientNameParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("recipientName").label("Recipient Name").defaultValue("").build();

iotwebconf::ParameterGroup EMAIL_group = iotwebconf::ParameterGroup("Email", "Email messages");
iotwebconf::TextTParameter<CONFIG_LEN> button1 = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("button1").label("Email text for button 1").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<CONFIG_LEN> button2 = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("button2").label("Email text for button 2").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<CONFIG_LEN> button3 = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("button3").label("Email text for button 3").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<CONFIG_LEN> button4 = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("button4").label("Email text for button 4").defaultValue("Button 1 has been pressed").build();

void smtpCallback(SMTP_Status status)
{
    logi("%s", status.info());
}

Notifier::Notifier()
{
    _smtp = new SMTPSession();
}

Notifier::~Notifier()
{
    delete _smtp;
}

String Notifier::getRootHTML() {
	String s;
	s += "<ul>";
	s += "<li>SMTP server: ";
	s += smtpServerParam.value();
	s += "</ul>";
	s += "<ul>";
	s += "<li>SMTP port: ";
	s += smtpPortParam.value();
	s += "</ul>";
	s += "<ul>";
	s += "<li>Sender Email: ";
	s += senderEmailParam.value();
	s += "</ul>";
	s += "<ul>";
	s += "<li>Sender password: ";
	// s += (strlen(senderPasswordParam.value()) > 0) ? "********" : "not set";
		s += senderPasswordParam.value();
	s += "</ul>";
	s += "<ul>";
	s += "<li>Recipient Email: ";
	s += recipientEmailParam.value();
	s += "</ul>";
	s += "<ul>";
	s += "<li>Recipient Name: ";
	s += recipientNameParam.value();
	s += "</ul>";
	return s;
}

iotwebconf::ParameterGroup* Notifier::parameterGroup() {
	return &SMTP_group;
}

bool Notifier::validate(iotwebconf::WebRequestWrapper* webRequestWrapper) {
	if ( requiredParam(webRequestWrapper, smtpServerParam) == false) return false;
	if ( requiredParam(webRequestWrapper, smtpPortParam) == false) return false;
	if ( requiredParam(webRequestWrapper, senderEmailParam) == false) return false;
	if (strlen(senderPasswordParam.value()) == 0) {
		if ( requiredParam(webRequestWrapper, senderPasswordParam) == false) return false;
	} else {
		webRequestWrapper->arg(senderPasswordParam.getId()) = "***************";
	}
	
	if ( requiredParam(webRequestWrapper, recipientEmailParam) == false) return false;
	return true;
}

void Notifier::setup(IOTServiceInterface* pcb){
    logd("setup");
	_pcb = pcb;
    _smtp->callback(smtpCallback);
    _smtp->setTCPTimeout(10);
	SMTP_group.addItem(&smtpServerParam);
	SMTP_group.addItem(&smtpPortParam);
	SMTP_group.addItem(&senderEmailParam);
	SMTP_group.addItem(&senderPasswordParam);
	SMTP_group.addItem(&recipientEmailParam);
	SMTP_group.addItem(&recipientNameParam);
	EMAIL_group.addItem(&button1);
	EMAIL_group.addItem(&button2);
	EMAIL_group.addItem(&button3);
	EMAIL_group.addItem(&button4);
	SMTP_group.addItem(&EMAIL_group);
}
void Notifier::notify(uint8_t pin){
	logi("Button %d has been pressed\n", pin);
	switch (pin){
		case BUTTON_1:
			sendit(button1.value());
			break;
		case BUTTON_2:
			sendit(button2.value());
			break;
		case BUTTON_3:
			sendit(button3.value());
			break;
		case BUTTON_4:
			sendit(button4.value());
			break;
	}
}

void Notifier::sendit(const char * content){
	logd("SMTP: %s:%s From:%s PW: %s To: (%s)%s", smtpServerParam.value(), smtpPortParam.value(), senderEmailParam.value(), senderPasswordParam.value(), recipientNameParam.value(), recipientEmailParam.value());
	ESP_Mail_Session session;
	session.server.host_name = smtpServerParam.value();

	session.server.port = atoi(smtpPortParam.value());
	session.login.email = senderEmailParam.value();
	session.login.password = senderPasswordParam.value();
	session.login.user_domain = "";
	/* Declare the message class */
	SMTP_Message message;
	message.sender.name = "ESP 32";
	message.sender.email = senderEmailParam.value();
	message.subject = "ESP32 Testing Email";
	message.addRecipient(recipientNameParam.value(), recipientEmailParam.value());
	message.timestamp.tag = "#esp_mail_current_time";
	message.timestamp.format = "%B %d, %Y %H:%M:%S";
	//Send HTML message
	String htmlMsg = "<div style=\"color:#000000;\"><h1>";
    htmlMsg.concat(content);
    htmlMsg.concat("</h1><p> Mail Generated from SwitchNotifier</p></div>");
	message.html.content = htmlMsg.c_str();
	message.text.charSet = "us-ascii";
	message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
	/* //Send simple text message
	String textMsg = "How are you doing";
	message.text.content = textMsg.c_str();
	message.text.charSet = "us-ascii";
	message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/
	if (!_smtp->connect(&session)) {
		loge("SMTP not connected");
		return;
	}
	if (!_smtp->isLoggedIn())
	{
		logw("SMTP not yet logged in.");
	}
	else
	{
	if (_smtp->isAuthenticated())
		logi("SMTP successfully logged in.");
	else
		logw("Connected with no Auth.");
	}
	if (!MailClient.sendMail(_smtp, &message))
		loge("SMTP Error, Status Code: %d, Error Code: %d, Reason: %s", _smtp->statusCode(), _smtp->errorCode(), _smtp->errorReason());
	}

} // namespace SwitchNotifier