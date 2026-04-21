#include "local_state_bridge.h"
#include "actuator_registry.h"
#include "audio_manager.h"

namespace {
bool g_firstQueryDone = false;
}

namespace LocalStateBridge {

void begin() {
  g_firstQueryDone = false;
}

void onPeriodicQuery() {
  if (!g_firstQueryDone) {
    AudioManager::speak(VoiceEvent::Query);
    g_firstQueryDone = true;
  }
}

void onQueryFailed() {
  AudioManager::speak(VoiceEvent::Fail);
}

void onActuatorStateObserved(int deviceId, uint16_t command, uint16_t standardStatus, uint32_t remainSec) {
  auto* prev = ActuatorRegistry::findByDeviceId(deviceId);
  uint16_t prevCmd = prev ? prev->command : 0xFFFF;
  bool prevOnline = prev ? prev->online : false;
  ActuatorRegistry::applyStateByDeviceId(deviceId, command, standardStatus, remainSec, true);
  if (prev && ((prevCmd != command) || !prevOnline)) {
    AudioManager::speak(VoiceEvent::Update);
  }
}

void onActuatorWritten(int deviceId, uint16_t command, uint16_t standardStatus, uint32_t remainSec) {
  ActuatorRegistry::applyStateByDeviceId(deviceId, command, standardStatus, remainSec, true);
  AudioManager::speak(VoiceEvent::Update);
}

}
