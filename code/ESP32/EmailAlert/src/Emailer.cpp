#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "Emailer.h"
#include "Log.h"


namespace EmailAlert
{

Emailer::Emailer()
{
    _smtp = new SMTPSession();
}

Emailer::~Emailer()
{
    delete _smtp;
}

void Emailer::setup(){
    logd("setup");
    //_smtp->callback(smtpCallback);
    _smtp->setTCPTimeout(10);
}


} // namespace EmailAlert