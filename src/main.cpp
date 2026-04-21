#include <Arduino.h>
#include "app_config.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "audio_manager.h"
#include "actuator_registry.h"
#include "local_state_bridge.h"
#include "server_poll_bridge.h"

namespace {
unsigned long g_lastUiRefresh = 0;

void printStates() {
  const auto* st = ActuatorRegistry::states();
  Serial.printf("==== ACTUATORS (%zu) ====\n", ActuatorRegistry::count());
  for (size_t i = 0; i < ActuatorRegistry::count(); ++i) {
    Serial.printf("  %s (id=%d) cmd=%u remain=%lu online=%d err=%s\n",
                  st[i].label.c_str(), st[i].deviceId,
                  (unsigned)st[i].command, (unsigned long)st[i].remainSec,
                  st[i].online ? 1 : 0,
                  ServerPollBridge::lastError().c_str());
  }
  Serial.println("========================");
}
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Farm Monitor (server polling) Start ===");

  DisplayManager::begin();
  DisplayManager::drawBoot();
  AudioManager::begin();
  WifiManager::begin();
  ActuatorRegistry::begin();
  LocalStateBridge::begin();
  ServerPollBridge::begin();
}

void loop() {
  WifiManager::loop();

  // Server polling — runs every SERVER_POLL_INTERVAL_MS when Wi-Fi is ready
  ServerPollBridge::loop();

  // Mark stale devices offline
  ActuatorRegistry::markOfflineIfStale(STALE_OFFLINE_MS);

  // Throttle UI redraws
  if (millis() - g_lastUiRefresh < UI_REFRESH_INTERVAL_MS) {
    delay(10);
    return;
  }
  g_lastUiRefresh = millis();

  const bool changed = ActuatorRegistry::consumeAnyDirty();
  if (!changed) {
    delay(10);
    return;
  }

  // Decide what to draw
  if (!WifiManager::ready()) {
    DisplayManager::drawError("Wi-Fi reconnecting...");
  } else if (!ActuatorRegistry::hasAnyOnline()) {
    DisplayManager::drawPolling(WifiManager::ip());
  } else {
    const String footer = ActuatorRegistry::footerText(
        WifiManager::ip(),
        ServerPollBridge::lastActuatorCount(),
        ServerPollBridge::lastError());
    DisplayManager::drawStates(ActuatorRegistry::states(),
                               ActuatorRegistry::count(),
                               footer);
  }

  printStates();
}
