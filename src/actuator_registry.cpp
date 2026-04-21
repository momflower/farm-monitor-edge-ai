#include "actuator_registry.h"
#include "app_config.h"

namespace {
ActuatorState g_states[MAX_ACTUATOR_REGISTRY_SIZE];
size_t g_count = 0;
}

namespace ActuatorRegistry {

void begin() {
  g_count = 0;
  // Pre-seed registry from config so labels are ready before first server poll.
  for (size_t i = 0; i < MONITORED_ACTUATOR_COUNT && g_count < MAX_ACTUATOR_REGISTRY_SIZE; ++i) {
    g_states[g_count] = ActuatorState{};
    g_states[g_count].deviceId = MONITORED_ACTUATORS[i].deviceId;
    g_states[g_count].label    = String(MONITORED_ACTUATORS[i].label);
    g_states[g_count].found    = true;
    g_states[g_count].online   = false;
    g_states[g_count].dirty    = true;
    ++g_count;
  }
}

size_t count() { return g_count; }
ActuatorState* mutableStates() { return g_states; }
const ActuatorState* states()  { return g_states; }

ActuatorState* findByDeviceId(int deviceId) {
  for (size_t i = 0; i < g_count; ++i) {
    if (g_states[i].deviceId == deviceId) return &g_states[i];
  }
  return nullptr;
}

ActuatorState* getOrCreate(int deviceId) {
  auto* s = findByDeviceId(deviceId);
  if (s) return s;
  if (g_count >= MAX_ACTUATOR_REGISTRY_SIZE) return nullptr;
  s = &g_states[g_count++];
  *s = ActuatorState{};
  s->deviceId = deviceId;
  s->dirty    = true;
  return s;
}

String statusText(uint16_t command) {
  switch (command) {
    case KsxCommand::STOP:        return "OFF";
    case KsxCommand::SWITCH_ON:   return "ON";
    case KsxCommand::SWITCH_TIME: return "T-ON";
    case KsxCommand::OPEN_ON:     return "OPEN";
    case KsxCommand::CLOSE_ON:    return "CLOSE";
    case KsxCommand::OPEN_TIME:   return "T-OPEN";
    case KsxCommand::CLOSE_TIME:  return "T-CLOSE";
    default:                      return String(command);
  }
}

void applyStateByDeviceId(int deviceId, uint16_t command, uint16_t standardStatus, uint32_t remainSec, bool online) {
  auto* s = findByDeviceId(deviceId);
  if (!s) return;

  const bool changed = (s->command        != command)        ||
                       (s->standardStatus != standardStatus) ||
                       (s->remainSec      != remainSec)      ||
                       (s->online         != online);
  s->command        = command;
  s->standardStatus = standardStatus;
  s->remainSec      = remainSec;
  s->online         = online;
  s->updatedAtMs    = millis();
  if (changed) s->dirty = true;
}

bool applyFromServer(int deviceId, const String& label, const String& name,
                     const String& zone, const String& type, const String& serverStatus,
                     uint16_t command, uint32_t remainSec) {
  auto* s = getOrCreate(deviceId);
  if (!s) return false;

  bool changed = false;

  // Update static metadata on first registration (or if previously blank)
  if (s->label.isEmpty()) { s->label = label; changed = true; }
  if (s->name.isEmpty())  { s->name  = name;  changed = true; }
  if (s->zone.isEmpty())  { s->zone  = zone; }
  if (s->type.isEmpty())  { s->type  = type; }

  // Detect operational state change
  changed |= (s->command      != command)      ||
             (s->remainSec    != remainSec)     ||
             (s->serverStatus != serverStatus)  ||
             (!s->online);

  s->command        = command;
  s->standardStatus = command;
  s->remainSec      = remainSec;
  s->serverStatus   = serverStatus;
  s->online         = true;
  s->found          = true;
  s->updatedAtMs    = millis();

  if (changed) s->dirty = true;
  return changed;
}

void markOfflineIfStale(unsigned long staleMs) {
  const unsigned long now = millis();
  for (size_t i = 0; i < g_count; ++i) {
    auto& s = g_states[i];
    if (s.updatedAtMs == 0) continue;
    if (s.online && now - s.updatedAtMs > staleMs) {
      s.online = false;
      s.dirty  = true;
    }
  }
}

bool consumeAnyDirty() {
  bool any = false;
  for (size_t i = 0; i < g_count; ++i) {
    if (g_states[i].dirty) { any = true; g_states[i].dirty = false; }
  }
  return any;
}

bool hasAnyOnline() {
  for (size_t i = 0; i < g_count; ++i) {
    if (g_states[i].online) return true;
  }
  return false;
}

String footerText(const String& ip, int pollCount, const String& pollErr) {
  String s = "IP " + ip + " | " + String(pollCount) + " devs";
  if (pollErr.length() > 0) s += " | ERR";
  return s;
}

}
