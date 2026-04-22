#pragma once
#include <Arduino.h>
#include "data_model.h"

namespace SensorPollBridge {

// Blocking one-shot fetch of the edge farm-sensor endpoint. Returns the
// parsed snapshot. On failure, `valid == false` and `error` holds the reason.
SensorSnapshot fetchOnce();

}
