#include <Arduino.h>
#include "app_config.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "audio_manager.h"
#include "actuator_registry.h"
#include "local_state_bridge.h"
#include "sensor_poll_bridge.h"
#include "server_poll_bridge.h"

namespace {
unsigned long g_lastUiRefresh  = 0;
bool          g_sensorSplashDone = false;
unsigned long g_wifiWaitStartMs  = 0;
constexpr unsigned long WIFI_WAIT_TIMEOUT_MS = 20000;  // give up splash after 20 s

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
  delay(500);
  Serial.println("\n=== Farm Monitor (server polling) Start ===");

  DisplayManager::begin();
  DisplayManager::drawGreeting();

  AudioManager::begin();
  delay(150);  // let amp settle after PA enable before the voiced greeting
  AudioManager::playGreetingVoice();

  DisplayManager::drawBoot();
  WifiManager::begin();
  ActuatorRegistry::begin();
  LocalStateBridge::begin();
  ServerPollBridge::begin();
  g_wifiWaitStartMs = millis();
}

void loop() {
  WifiManager::loop();

  // One-shot sensor splash — runs after Wi-Fi connects, blocks for
  // SENSOR_SPLASH_MS so the operator can read the values, then hands off
  // to normal actuator polling.
  if (!g_sensorSplashDone) {
    if (WifiManager::ready()) {
      Serial.println("[MAIN] sensor splash begin");
      const SensorSnapshot snap = SensorPollBridge::fetchOnce();
      DisplayManager::drawSensors(snap);

      // Announce "온도가 높아요" while the screen is visible. Voice is ~1 s,
      // pad the remainder so total splash time equals SENSOR_SPLASH_MS.
      const unsigned long tSplashStart = millis();
      AudioManager::playTempHighVoice();
      const unsigned long voiceElapsed = millis() - tSplashStart;
      if (voiceElapsed < SENSOR_SPLASH_MS) {
        delay(SENSOR_SPLASH_MS - voiceElapsed);
      }

      Serial.println("[MAIN] sensor splash done");
      g_sensorSplashDone = true;
    } else if (millis() - g_wifiWaitStartMs > WIFI_WAIT_TIMEOUT_MS) {
      Serial.println("[MAIN] sensor splash skipped — Wi-Fi timeout");
      g_sensorSplashDone = true;
    } else {
      delay(50);
      return;   // skip actuator polling until splash resolves one way or the other
    }
  }

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