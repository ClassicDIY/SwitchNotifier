#include <Arduino.h>
#include <SPI.h>
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
	logd("Done setup");
}

void loop()
{
	if (_iot.Run()) {
		if (_lastPublishTimeStamp < millis())
		{
			feed_watchdog();
			_notifier.run();
			_lastPublishTimeStamp = millis() + _currentPublishRate;
		}
	}
	else {
		feed_watchdog(); // don't reset when not configured
	}
}

