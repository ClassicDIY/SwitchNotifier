#include <Arduino.h>
#include <SPI.h>
#include <ESP_Mail_Client.h>
#include "Log.h"
#include "Defines.h"
#include "IOT.h"
#include "Emailer.h"

using namespace EmailAlert;

EmailAlert::IOT _iot = EmailAlert::IOT();
hw_timer_t *_watchdogTimer = NULL;

unsigned long _lastPublishTimeStamp = 0;
unsigned long _currentPublishRate = WAKE_PUBLISH_RATE; // rate currently being used
unsigned long _wakePublishRate = WAKE_PUBLISH_RATE; // wake publish rate set by config or mqtt command
boolean _stayAwake = false;
int _publishCount = 0;
boolean _sentEmail = false;

SMTPSession smtp;
void smtpCallback(SMTP_Status status);

void sendit(){

    smtp.callback(smtpCallback);
    smtp.setTCPTimeout(10);
	ESP_Mail_Session session;
	session.server.host_name = SMTP_server ;
	session.server.port = SMTP_Port;
	session.login.email = sender_email;
	session.login.password = sender_password;
	session.login.user_domain = "";
	/* Declare the message class */
	SMTP_Message message;
	message.sender.name = "ESP 32";
	message.sender.email = sender_email;
	message.subject = "ESP32 Testing Email";
	message.addRecipient(Recipient_name,Recipient_email);
	message.timestamp.tag = "#esp_mail_current_time";
	message.timestamp.format = "%B %d, %Y %H:%M:%S";
	//Send HTML message
	String htmlMsg = "<div style=\"color:#000000;\"><h1> Alert from Sensor!</h1><p> Mail Generated from EmailAlert</p></div>";
	message.html.content = htmlMsg.c_str();
	message.text.charSet = "us-ascii";
	message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
	/* //Send simple text message
	String textMsg = "How are you doing";
	message.text.content = textMsg.c_str();
	message.text.charSet = "us-ascii";
	message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/
	if (!smtp.connect(&session)) {
		loge("SMTP not connected");
		return;
	}
	if (!smtp.isLoggedIn())
	{
		logw("SMTP not yet logged in.");
	}
	else
	{
	if (smtp.isAuthenticated())
		logi("SMTP successfully logged in.");
	else
		logw("Connected with no Auth.");
	}
	if (!MailClient.sendMail(&smtp, &message))
		loge("SMTP Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason());
	}


void IRAM_ATTR resetModule()
{
	// ets_printf("watchdog timer expired - rebooting\n");
	esp_restart();
}

void init_watchdog()
{
	if (_watchdogTimer == NULL)
	{
		_watchdogTimer = timerBegin(0, 80, true);					   //timer 0, div 80
		timerAttachInterrupt(_watchdogTimer, &resetModule, true);	  //attach callback
		timerAlarmWrite(_watchdogTimer, WATCHDOG_TIMER * 1000, false); //set time in us
		timerAlarmEnable(_watchdogTimer);							   //enable interrupt
	}
}

void feed_watchdog()
{
	if (_watchdogTimer != NULL)
	{
		timerWrite(_watchdogTimer, 0); // feed the watchdog
	}
}

void Wake()
{
	_currentPublishRate = _wakePublishRate;
	_lastPublishTimeStamp = 0;
	_publishCount = 0;
}

void setup()
{
	MQTTCommandInterface* tmp;
	Serial.begin(115200);
	while (!Serial) {}
	logd("Booting");
	_iot.Init(tmp);
	init_watchdog();
	_lastPublishTimeStamp = millis() + WAKE_PUBLISH_RATE;
	logd("Done setup");

}

void loop()
{
	if (_iot.Run()) {
		
		if (_lastPublishTimeStamp < millis())
		{
			logd("loop");
			feed_watchdog();
			// _ts45.run(); 
			if (!_sentEmail) {
				_sentEmail = true;
				sendit();
			}
			_lastPublishTimeStamp = millis() + _iot.PublishRate();
		}
	}
	else {
		feed_watchdog(); // don't reset when not configured
	}
}

void smtpCallback(SMTP_Status status)
{
    logi("%s", status.info());
    if (status.success())
    {
        logi("----------------");
        logi("Message sent success: %d", status.completedCount());
        logi("Message sent failed: %d", status.failedCount());
        logi("----------------");
        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            SMTP_Result result = smtp.sendingResult.getItem(i);
            logi("Message No: %d", i + 1);
            logi("Status: %s", result.completed ? "success" : "failed");
            logi("Date/Time: %s", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            logi("Recipient: %s", result.recipients.c_str());
            logi("Subject: %s", result.subject.c_str());
        }
        logi("----------------");
        smtp.sendingResult.clear();
    }
}