#pragma once
#include <Arduino.h>

// Call these from your existing KSX control/poll flow.
namespace LocalStateBridge {
void begin();
void onPeriodicQuery();
void onQueryFailed();
void onActuatorStateObserved(int deviceId, uint16_t command, uint16_t standardStatus = 0, uint32_t remainSec = 0);
void onActuatorWritten(int deviceId, uint16_t command, uint16_t standardStatus = 0, uint32_t remainSec = 0);
}
