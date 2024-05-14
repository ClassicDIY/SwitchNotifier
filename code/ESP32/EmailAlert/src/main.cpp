#include <Arduino.h>
#include <SPI.h>
#include <ESP_Mail_Client.h>
#include "Log.h"
#include "Defines.h"
#include "IOT.h"
#include "Notifier.h"

using namespace SwitchNotifier;

SwitchNotifier::IOT _iot = SwitchNotifier::IOT();
SwitchNotifier::Notifier _notifier = SwitchNotifier::Notifier();
hw_timer_t *_watchdogTimer = NULL;

unsigned long _lastPublishTimeStamp = 0;
unsigned long _currentPublishRate = WAKE_PUBLISH_RATE; // rate currently being used
unsigned long _wakePublishRate = WAKE_PUBLISH_RATE; // wake publish rate set by config or mqtt command
boolean _stayAwake = false;
char msgBuffer[STR_LEN];

struct Button {
	const uint8_t PIN;
	bool pressed;
};
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
}

void setup()
{
	MQTTCallbackInterface* tmp;
	Serial.begin(115200);
	while (!Serial) {}
	logd("Booting");
	_notifier.setup(&_iot);
	_iot.Init(&_notifier, tmp);
	
	init_watchdog();
	_lastPublishTimeStamp = millis() + WAKE_PUBLISH_RATE;
	pinMode(BUTTON_1, INPUT_PULLUP);
	attachInterrupt(button1.PIN, isr1, FALLING);
	pinMode(BUTTON_2, INPUT_PULLUP);
	attachInterrupt(button2.PIN, isr2, FALLING);
	pinMode(BUTTON_3, INPUT_PULLUP);
	attachInterrupt(button3.PIN, isr3, FALLING);
	pinMode(BUTTON_4, INPUT_PULLUP);
	attachInterrupt(button4.PIN, isr4, FALLING);
	logd("Done setup");
}

void monitorButton(Button& button) {
	if (button.pressed) {
		_notifier.notify(button.PIN);
		button.pressed = false;
	}
}

void loop()
{
	if (_iot.Run()) {
		if (_lastPublishTimeStamp < millis())
		{
			feed_watchdog();
			monitorButton(button1);
			monitorButton(button2);
			monitorButton(button3);
			monitorButton(button4);
			_lastPublishTimeStamp = millis() + _currentPublishRate;
		}
	}
	else {
		feed_watchdog(); // don't reset when not configured
	}
}

