#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "HelperFunctions.h"
#include "IOT.h"
#include "Notifier.h"
#include "Log.h"
#include "style.html"
#include "home.html"

namespace CLASSICDIY
{

	static AsyncWebServer _asyncServer(ASYNC_WEBSERVER_PORT);
	static AsyncWebSocket _webSocket("/ws_home");
	IOT _iot = IOT();

	Notifier::Notifier()
	{
		_smtp = new SMTPSession();
	}

	Notifier::~Notifier()
	{
		delete _smtp;
	}
	static const char smtpPorts[][CONFIG_LEN] = {"25", "465", "587", "2525"};

	void smtpCallback(SMTP_Status status)
	{
		logi("%s", status.info());
	}

	void Notifier::onSaveSetting(JsonDocument &doc)
	{
		JsonObject messages = doc["messages"].to<JsonObject>();
		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			String msg = "msg" + i;
			messages[msg] = _buttons[i].message;
		}
		String jsonString;
		serializeJson(doc, jsonString);
		ets_printf("%s\r\n", jsonString.c_str());
	}

	void Notifier::onLoadSetting(JsonDocument &doc)
	{
		JsonObject messages = doc["messages"].as<JsonObject>();
		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			String msg = "msg" + i;
			_buttons[i].message = messages[msg].as<std::string>();
		}
		String jsonString;
		serializeJson(doc, jsonString);
		ets_printf("%s\r\n", jsonString.c_str());
	}

	void Notifier::addApplicationConfigs(String &page)
	{
		String msgFields = app_config_fields;
		String msg_flds;

		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			String msg_fld = app_message;
			msg_fld.replace("{bm}", String(i));
			msg_fld.replace("{message}", _buttons[i].message.c_str());
			msg_flds += msg_fld;
		}
		msgFields.replace("{aconv}", msg_flds);
		page += msgFields;
	}

	void Notifier::onSubmitForm(AsyncWebServerRequest *request)
	{
		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			String msgn = "message" + String(i);
			if (request->hasParam(msgn, true))
			{
				_buttons[i].message = request->getParam(msgn, true)->value().c_str();
			}
		}
	}

	void Notifier::setup()
	{
		logd("setup");
		_iot.Init(this, &_asyncServer);
		_smtp->callback(smtpCallback);
		_smtp->setTCPTimeout(10);
		gpio_install_isr_service(ESP_INTR_FLAG_LOWMED); 
		for (int i = 0; i < NUM_BUTTONS; ++i) {
			_buttons[i].init();
		}

		_asyncServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
						{
			String page = home_html;
			std::string s;
			page.replace("{style}", style);
			page.replace("{n}", _iot.getThingName().c_str());
			page.replace("{v}", APP_VERSION);
			char desc_buf[64];
			sprintf(desc_buf, "SMTP Server: %s", _smtpServer.c_str());
			page.replace("{stmpServer}", desc_buf);
			sprintf(desc_buf, "SMTP Port: %d", _smtpPort);
			page.replace("{stmpPort}", desc_buf);
// ToDo...

			for (int i = 0; i < NUM_BUTTONS; i++)
			{
				s += "<div class='desc' >";
				s += " Button";
				s += std::to_string(i);
				s += ": ";
				s += _buttons[i].message.c_str();
				s += "</div>";
			}
			page.replace("{emailMessages}", s.c_str());

			request->send(200, "text/html", page); });
		_asyncServer.addHandler(&_webSocket).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next)
														   {
			// ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
			if (_webSocket.count() > 1) 
			{
				// if we have 2 clients or more, prevent the next one to connect
				request->send(503, "text/plain", "Server is busy");
			} else 
			{
				// process next middleware and at the end the handler
				next();
			} });
		_webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
						   {
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
        	} });
	}

	void Notifier::run()
	{
		_iot.Run();
		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			if (_buttons[i].pressed) {
				_buttons[i].pressed = false;
				sendit(_buttons[i].message);
			}
		}
	}

	void Notifier::sendit(std::string content)
	{
		logd("sendit: %s", content.c_str());
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
		//Send HTML message
		String htmlMsg = "<div style=\"color:#000000;\"><h1>";
		htmlMsg.concat(content.c_str());
		htmlMsg.concat("</h1><p> Mail Generated from SwitchNotifier</p></div>");
		message.html.content = htmlMsg.c_str();
		message.text.charSet = "us-ascii";
		message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
		/* //Send simple text message
		String textMsg = "How are you doing";
		message.text.content = textMsg.c_str();
		message.text.charSet = "us-ascii";
		message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/
		#if TINY_GSM_MODEM_SIM7600
		_pcb->setGSMClient(_smtp);
		#endif
		if (!_smtp->connect(&session, true)) {
			loge("SMTP not connected");
			return;
		}
		if (!_smtp->isLoggedIn()) {
			logw("SMTP not yet logged in.");
		}
		else {
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

} // namespace SwitchNotifier