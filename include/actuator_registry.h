#pragma once
#include <Arduino.h>
#include "data_model.h"

namespace ActuatorRegistry {
void begin();
size_t count();
ActuatorState* mutableStates();
const ActuatorState* states();
ActuatorState* findByDeviceId(int deviceId);
ActuatorState* getOrCreate(int deviceId);

// Called by LocalStateBridge (KSX integration)
void applyStateByDeviceId(int deviceId, uint16_t command, uint16_t standardStatus, uint32_t remainSec, bool online = true);

// Called by ServerPollBridge (HTTP polling)
// Returns true if any field changed for this device.
bool applyFromServer(int deviceId, const String& label, const String& name,
                     const String& zone, const String& type, const String& serverStatus,
                     uint16_t command, uint32_t remainSec);

void markOfflineIfStale(unsigned long staleMs);
bool consumeAnyDirty();
bool hasAnyOnline();
String footerText(const String& ip, int pollCount, const String& pollErr);
String statusText(uint16_t command);
}
