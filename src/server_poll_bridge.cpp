#include "server_poll_bridge.h"
#include "actuator_registry.h"
#include "audio_manager.h"
#include "app_config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// POST body exactly as the main controller uses:
//   deviceIndexList=222,223,224,225,226,227,228,229,230
static String buildDeviceIndexList() {
  String s = "deviceIndexList=";
  for (size_t i = 0; i < MONITORED_ACTUATOR_COUNT; ++i) {
    if (i > 0) s += ",";
    s += String(MONITORED_ACTUATORS[i].deviceId);
  }
  return s;
}

// Map server valueAct / deviceType / offset / period → KSX command code.
// Mirrors resolveMotorKsxCommandByOffset() logic from the main controller.
static uint16_t toKsxCommand(int devTypeId, int actValue, int offset, int valuePeriod) {
  if (devTypeId == 2) {  // MOTOR
    if (actValue == 1) return (abs(offset) >= 10) ? KsxCommand::OPEN_ON : KsxCommand::OPEN_TIME;
    if (actValue == 2) return (abs(offset) >= 10) ? KsxCommand::CLOSE_ON : KsxCommand::CLOSE_TIME;
    return KsxCommand::STOP;
  }
  // RELAY (devTypeId == 1)
  if (actValue == 1) return (valuePeriod > 0) ? KsxCommand::SWITCH_TIME : KsxCommand::SWITCH_ON;
  return KsxCommand::STOP;
}

// Remaining seconds for display:
//   switch with period  → valuePeriod
//   motor timed         → grad × |offset|
static uint32_t toRemainSec(uint16_t cmd, int grad, int offset, int valuePeriod) {
  if (cmd == KsxCommand::SWITCH_TIME)                                  return (uint32_t)valuePeriod;
  if (cmd == KsxCommand::OPEN_TIME || cmd == KsxCommand::CLOSE_TIME)  return (uint32_t)(grad * abs(offset));
  return 0;
}

namespace {
unsigned long g_lastPollMs  = 0;
String        g_lastError;
int           g_lastCount   = 0;
bool          g_firstPoll   = true;

// Voice announcements are produced for these switches (in monitored order).
// Index 0 → SW1 (spoken as "1번"), index 1 → SW2, index 2 → SW3.
constexpr int VOICE_SWITCH_IDS[] = {222, 223, 224};
constexpr int VOICE_SWITCH_COUNT = sizeof(VOICE_SWITCH_IDS) / sizeof(VOICE_SWITCH_IDS[0]);
int g_swLastCmd[VOICE_SWITCH_COUNT];   // -1 = unknown; 0 = off (STOP); 1 = on (any non-STOP)
}

namespace ServerPollBridge {

void begin() {
  g_lastPollMs = 0;
  g_lastError  = "";
  g_lastCount  = 0;
  g_firstPoll  = true;
  for (int i = 0; i < VOICE_SWITCH_COUNT; ++i) g_swLastCmd[i] = -1;
}

void loop() {
  if (!WifiManager::ready()) return;
  if (!g_firstPoll && millis() - g_lastPollMs < SERVER_POLL_INTERVAL_MS) return;

  g_lastPollMs = millis();
  if (g_firstPoll) AudioManager::speak(VoiceEvent::Query);

  // ── POST ──────────────────────────────────────────────────────────────────
  HTTPClient http;
  const String url  = makeServerUrl(API_DEVICE_COMMAND_POST);
  const String body = buildDeviceIndexList();

  Serial.println("[POLL] POST " + url);
  Serial.println("[POLL] body: " + body);

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("AppKey", APP_KEY);
  http.setTimeout(HTTP_TIMEOUT_MS);

  const int code = http.POST(body);
  String payload;
  if (code == HTTP_CODE_OK) payload = http.getString();
  http.end();

  if (code != HTTP_CODE_OK) {
    g_lastError = "HTTP " + String(code);
    Serial.println("[POLL] Error: " + g_lastError);
    AudioManager::speak(VoiceEvent::Fail);
    return;
  }
  Serial.printf("[POLL] %d bytes: %.120s\n", payload.length(), payload.c_str());

  // ── JSON parse ────────────────────────────────────────────────────────────
  // Response is always a JSON array (empty [] when no active commands).
  JsonDocument doc;
  const DeserializationError jerr = deserializeJson(doc, payload);
  if (jerr || !doc.is<JsonArray>()) {
    g_lastError = jerr ? String("JSON:") + jerr.c_str() : "not array";
    Serial.println("[POLL] " + g_lastError);
    AudioManager::speak(VoiceEvent::Fail);
    return;
  }

  JsonArray arr = doc.as<JsonArray>();

  // ── Apply to registry ─────────────────────────────────────────────────────
  // Track which monitored IDs appeared in the response.
  bool found[MONITORED_ACTUATOR_COUNT];
  memset(found, 0, sizeof(found));

  bool anyChange = false;
  g_lastCount    = 0;

  for (JsonObject obj : arr) {
    const int id = obj["id"] | 0;
    if (id <= 0) continue;

    const int devTypeId   = obj["deviceType"]["id"] | 1;
    const int actValue    = obj["valueAct"]       | 0;
    const int offset      = obj["valueOffset"]    | 0;
    const int grad        = obj["valueGradient"]  | 0;
    const int valuePeriod = obj["valuePeriod"]    | 0;

    const uint16_t cmd    = toKsxCommand(devTypeId, actValue, offset, valuePeriod);
    const uint32_t remain = toRemainSec(cmd, grad, offset, valuePeriod);

    const bool isMotor = (devTypeId == 2);
    const String type  = isMotor ? "MOTOR" : "RELAY";
    // UTF-8: 개폐기 / 스위치
    const String zone  = isMotor
        ? "\xEA\xB0\x9C\xED\x8F\x90\xEA\xB8\xB0"
        : "\xEC\x8A\xA4\xEC\x9C\x84\xEC\xB9\x98";

    // Use pre-seeded label; mark as found in MONITORED_ACTUATORS
    String label = "D" + String(id % 100);
    for (size_t i = 0; i < MONITORED_ACTUATOR_COUNT; ++i) {
      if (MONITORED_ACTUATORS[i].deviceId == id) {
        label    = String(MONITORED_ACTUATORS[i].label);
        found[i] = true;
        break;
      }
    }

    anyChange |= ActuatorRegistry::applyFromServer(id, label, "", zone, type, "normal", cmd, remain);
    ++g_lastCount;

    Serial.printf("[POLL]  id=%d type=%d act=%d offset=%d grad=%d period=%d → cmd=%u remain=%lu\n",
                  id, devTypeId, actValue, offset, grad, valuePeriod,
                  (unsigned)cmd, (unsigned long)remain);
  }

  // Devices absent from the response have no active command → STOP
  for (size_t i = 0; i < MONITORED_ACTUATOR_COUNT; ++i) {
    if (found[i]) continue;
    const int  id      = MONITORED_ACTUATORS[i].deviceId;
    const bool isMotor = (id >= 228);   // 228-230 are openers
    const String type  = isMotor ? "MOTOR" : "RELAY";
    const String zone  = isMotor
        ? "\xEA\xB0\x9C\xED\x8F\x90\xEA\xB8\xB0"
        : "\xEC\x8A\xA4\xEC\x9C\x84\xEC\xB9\x98";
    anyChange |= ActuatorRegistry::applyFromServer(
        id, String(MONITORED_ACTUATORS[i].label), "", zone, type,
        "normal", KsxCommand::STOP, 0);
  }

  // SW1-SW3 spoken on/off transitions. First observation is silent so the
  // current state right after boot doesn't re-announce itself.
  bool anySwitchSpoke = false;
  for (int i = 0; i < VOICE_SWITCH_COUNT; ++i) {
    const ActuatorState* s = ActuatorRegistry::findByDeviceId(VOICE_SWITCH_IDS[i]);
    if (!s) continue;
    const int now = (s->command != KsxCommand::STOP) ? 1 : 0;
    if (g_swLastCmd[i] != -1 && g_swLastCmd[i] != now) {
      AudioManager::playSwitchStateVoice(i + 1, now == 1);
      anySwitchSpoke = true;
    }
    g_swLastCmd[i] = now;
  }

  // Generic update tone covers any change that wasn't already spoken.
  if (anyChange && !anySwitchSpoke) AudioManager::speak(VoiceEvent::Update);

  g_lastError = "";
  g_firstPoll = false;
  Serial.printf("[POLL] Done: %d active, changed=%d\n", g_lastCount, anyChange ? 1 : 0);
}

String lastError()         { return g_lastError; }
int    lastActuatorCount() { return g_lastCount;  }

}
