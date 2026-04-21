#pragma once
#include <Arduino.h>
#include "data_model.h"

namespace DisplayManager {
void begin();
void drawBoot();
void drawPolling(const String& ip);  // waiting for first server response
void drawError(const String& msg);
void drawStates(const ActuatorState* states, size_t count, const String& footer);
}
