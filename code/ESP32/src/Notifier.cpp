#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "HelperFunctions.h"
#include "Notifier.h"
#include "Log.h"

namespace SwitchNotifier
{
static const char smtpPorts[][CONFIG_LEN] = { "25", "465", "587", "2525" };
iotwebconf::ParameterGroup Notifier_group = iotwebconf::ParameterGroup("Notifier", "Notifier");
iotwebconf::ParameterGroup SMTP_group = iotwebconf::ParameterGroup("Email", "SMTP");
iotwebconf::TextTParameter<CONFIG_LEN> smtpServerParam = iotwebconf::Builder<iotwebconf::TextTParameter<CONFIG_LEN>>("smtpServer").label("SMTP server").defaultValue(SMTP_server).build();
iotwebconf::SelectTParameter<CONFIG_LEN> smtpPortParam = iotwebconf::Builder<iotwebconf::SelectTParameter<CONFIG_LEN>>("smtpPort").label("SMTP port").
	optionValues((const char*)smtpPorts).optionNames((const char*)smtpPorts).optionCount(sizeof(smtpPorts) / CONFIG_LEN).nameLength(CONFIG_LEN).defaultValue("465").build();
iotwebconf::TextTParameter<STR_LEN> senderEmailParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("senderEmail").label("Sender Email").defaultValue("").build();
iotwebconf::PasswordTParameter<STR_LEN> senderPasswordParam = iotwebconf::Builder<iotwebconf::PasswordTParameter<STR_LEN>>("senderPassword").label("Sender password").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> recipientEmailParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("recipientEmail").label("Recipient Email").defaultValue("").build();
iotwebconf::TextTParameter<STR_LEN> recipientNameParam = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("recipientName").label("Recipient Name").defaultValue("").build();

iotwebconf::ParameterGroup Button_Group = iotwebconf::ParameterGroup("Button_Group", "Email messages");
iotwebconf::TextTParameter<STR_LEN> buttonParam1 = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("buttonParam1").label("Email text for button 1").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<STR_LEN> buttonParam2 = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("buttonParam2").label("Email text for button 2").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<STR_LEN> buttonParam3 = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("buttonParam3").label("Email text for button 3").defaultValue("Button 1 has been pressed").build();
iotwebconf::TextTParameter<STR_LEN> buttonParam4 = iotwebconf::Builder<iotwebconf::TextTParameter<STR_LEN>>("buttonParam4").label("Email text for button 4").defaultValue("Button 1 has been pressed").build();


Button button1 = {BUTTON_1, false};
Button button2 = {BUTTON_2, false};
Button button3 = {BUTTON_3, false};
Button button4 = {BUTTON_4, false};

void IRAM_ATTR isr1() {
	button1.pressed = true;
}
void IRAM_ATTR isr2() {
	button2.pressed = true;
}
void IRAM_ATTR isr3() {
	button3.pressed = true;
}
void IRAM_ATTR isr4() {
	button4.pressed = true;
}


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
	s += "Email:";
	s += "<ul>";
	s += "<li>SMTP server: ";
	s += smtpServerParam.value();
	s += "<li>SMTP port: ";
	s += smtpPortParam.value();
	s += "<li>Sender Email: ";
	s += senderEmailParam.value();
	s += "<li>Sender password: ";
	s += (strlen(senderPasswordParam.value()) > 0) ? "********" : "not set";
	s += "<li>Recipient Email: ";
	s += recipientEmailParam.value();
	s += "<li>Recipient Name: ";
	s += recipientNameParam.value();
	s += "</ul>";
	s += "Messages:";
	s += "<ul>";
	s += "<li>Message 1: ";
	s += buttonParam1.value();
	s += "<li>Message 2: ";
	s += buttonParam2.value();
	s += "<li>Message 3: ";
	s += buttonParam3.value();
	s += "<li>Message 4: ";
	s += buttonParam4.value();
	s += "</ul>";
	return s;
}

iotwebconf::ParameterGroup* Notifier::parameterGroup() {
	return &Notifier_group;
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
	Button_Group.addItem(&buttonParam1);
	Button_Group.addItem(&buttonParam2);
	Button_Group.addItem(&buttonParam3);
	Button_Group.addItem(&buttonParam4);
	Notifier_group.addItem(&SMTP_group);
	Notifier_group.addItem(&Button_Group);
	pinMode(BUTTON_1, INPUT_PULLUP);
	attachInterrupt(button1.PIN, isr1, FALLING);
	pinMode(BUTTON_2, INPUT_PULLUP);
	attachInterrupt(button2.PIN, isr2, FALLING);
	pinMode(BUTTON_3, INPUT_PULLUP);
	attachInterrupt(button3.PIN, isr3, FALLING);
	pinMode(BUTTON_4, INPUT_PULLUP);
	attachInterrupt(button4.PIN, isr4, FALLING);
}

void Notifier::monitorButton(Button& button) {
	if (button.pressed) {
		notify(button.PIN);
		button.pressed = false;
	}
}
void Notifier::run(){
	monitorButton(button1);
	monitorButton(button2);
	monitorButton(button3);
	monitorButton(button4);
}

void Notifier::notify(uint8_t pin){
	logi("Button %d has been pressed\n", pin);
	JsonDocument doc;
	doc["TimeStamp"] = getTime();
	switch (pin){
		case BUTTON_1:
			sendit(buttonParam1.value());
			_pcb->Publish("Switch_1", doc, false);
			break;
		case BUTTON_2:
			sendit(buttonParam2.value());
			_pcb->Publish("Switch_2", doc, false);
			break;
		case BUTTON_3:
			sendit(buttonParam3.value());
			_pcb->Publish("Switch_3",doc, false);
			break;
		case BUTTON_4:
			sendit(buttonParam4.value());
			_pcb->Publish("Switch_4", doc, false);
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