#pragma once
#include <Arduino.h>

// Periodically GETs the actuator list from the server and updates ActuatorRegistry.
// Call begin() once in setup() and loop() every iteration of Arduino loop().
namespace ServerPollBridge {
void begin();
void loop();
String lastError();        // empty if last poll succeeded
int    lastActuatorCount();// number of actuators received in last poll
}
