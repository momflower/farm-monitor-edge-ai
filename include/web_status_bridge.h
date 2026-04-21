#pragma once
#include <Arduino.h>

// Existing outbound server update/reporting functions.
// Keep these attached to the old control flow when needed.
namespace WebStatusBridge {
void begin();
bool updateDeviceStatus(int deviceId, int actValue, int statusDevice);
bool postDeviceStandardStatusToServer(int deviceId, uint16_t statusIndex);
bool postDeviceBoardStatusToServer(int farmIndex, uint16_t statusIndex);
bool reportActuatorStatusToServer(int deviceId, int state);
}
