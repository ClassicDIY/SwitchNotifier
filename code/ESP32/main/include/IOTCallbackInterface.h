#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>
#include "Enumerations.h"

namespace CLASSICDIY {

class IOTCallbackInterface {
 public:
   virtual void onNetworkState(NetworkState state) = 0;
   virtual String appTemplateProcessor(const String &var);
   virtual void onSaveSetting(JsonDocument &doc);
   virtual void onLoadSetting(JsonDocument &doc);
};

} // namespace CLASSICDIY