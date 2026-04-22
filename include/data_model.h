#pragma once
#include <Arduino.h>

struct ActuatorState {
  int deviceId = 0;
  String label;         // Short display label (SW1, OP1, …)
  String name;          // Full name from server (스위치1, 개폐기2, …)
  String zone;          // Zone from server (스위치, 개폐기, …)
  String type;          // Type from server (RELAY, MOTOR, …)
  String serverStatus;  // Server health status (정상, 이상, …)
  uint16_t command = 0;         // KSX operational command (0/201/301/…)
  uint16_t standardStatus = 0;  // status_index if available
  uint32_t remainSec = 0;       // Remaining seconds for timed commands
  bool found = false;
  bool online = false;
  bool dirty = false;
  unsigned long updatedAtMs = 0;
};

enum class VoiceEvent {
  None,
  Query,   // Periodic poll started
  Update,  // State change detected
  Fail,    // Poll / parse error
};

// One-shot sensor snapshot shown during boot splash.
// NaN means "no reading" / "sensor missing from payload".
struct SensorSnapshot {
  float temp1 = NAN;
  float temp2 = NAN;
  float hum1  = NAN;
  float hum2  = NAN;
  float rain  = NAN;
  float flow  = NAN;
  bool   valid = false;
  String farmName;
  String error;
};
