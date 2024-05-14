#pragma once
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include "Defines.h"

boolean inline requiredParam(iotwebconf::WebRequestWrapper* webRequestWrapper, iotwebconf::InputParameter& param) {
	boolean valid = true;
	int paramLength = webRequestWrapper->arg(param.getId()).length();
	if (paramLength == 0)
	{
		param.errorMessage = "This field is required\n";
		valid = false;
	}
	return valid;
}