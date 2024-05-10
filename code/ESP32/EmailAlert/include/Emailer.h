#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "log.h"
#include "Defines.h"

namespace EmailAlert
{

//void smtpCallback(SMTP_Status status);
// void smtpCallback(SMTP_Status status)
// {
//     logi("%s", status.info());
// }

class Emailer
{
 public:
 	Emailer();
	~Emailer();
    void setup();

 protected:
    SMTPSession* _smtp;
};

} // namespace EmailAlert