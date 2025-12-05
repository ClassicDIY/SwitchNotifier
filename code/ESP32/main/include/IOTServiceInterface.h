#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"

namespace CLASSICDIY {

class IOTServiceInterface {
 public:
   virtual u_int getUniqueId() = 0;
   virtual std::string getThingName() = 0;
};

} // namespace CLASSICDIY