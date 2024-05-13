#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "Emailer.h"
#include "Log.h"


namespace EmailAlert
{

void smtpCallback(SMTP_Status status)
{
    logi("%s", status.info());
}

Emailer::Emailer()
{
    _smtp = new SMTPSession();
}

Emailer::~Emailer()
{
    delete _smtp;
}

void Emailer::setup(IOTCallbackInterface* pcb){
    logd("setup");
	_pcb = pcb;
    _smtp->callback(smtpCallback);
    _smtp->setTCPTimeout(10);
}

void Emailer::sendit(const char * content){
	logd("SMTP: %s:%d PW: %s From:%s To: (%s)%s", _pcb->getSMTPServer(), _pcb->getSMTPPort(), _pcb->getSenderPassword(), _pcb->getSenderEmail(), _pcb->getRecipientName(), _pcb->getRecipientEmail());
	ESP_Mail_Session session;
	session.server.host_name = _pcb->getSMTPServer();

	session.server.port = _pcb->getSMTPPort();
	session.login.email = _pcb->getSenderEmail();
	session.login.password = _pcb->getSenderPassword();
	session.login.user_domain = "";
	/* Declare the message class */
	SMTP_Message message;
	message.sender.name = "ESP 32";
	message.sender.email = _pcb->getSenderEmail();
	message.subject = "ESP32 Testing Email";
	message.addRecipient(_pcb->getRecipientName(), _pcb->getRecipientEmail());
	message.timestamp.tag = "#esp_mail_current_time";
	message.timestamp.format = "%B %d, %Y %H:%M:%S";
	//Send HTML message
	String htmlMsg = "<div style=\"color:#000000;\"><h1>";
    htmlMsg.concat(content);
    htmlMsg.concat("</h1><p> Mail Generated from EmailAlert</p></div>");
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

} // namespace EmailAlert