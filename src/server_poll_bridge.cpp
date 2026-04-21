#include "server_poll_bridge.h"
#include "actuator_registry.h"
#include "audio_manager.h"
#include "app_config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Derive a short ASCII label from the server-supplied Korean name.
// "스위치N" → "SWN",  "개폐기N" → "OPN",  fallback → type-prefix + id%100
static String generateLabel(const String& name, const String& type, int deviceId) {
  // UTF-8: 스위치 = EC 8A A4 EC 9C 84 EC B9 98 (9 bytes)
  //        개폐기 = EA B0 9C ED 8F 90 EA B8 B0 (9 bytes)
  if (name.length() >= 10) {
    const String prefix9 = name.substring(0, 9);
    const String suffix  = name.substring(9);
    if (prefix9 == "\xEC\x8A\xA4\xEC\x9C\x84\xEC\xB9\x98") return "SW" + suffix;  // 스위치
    if (prefix9 == "\xEA\xB0\x9C\xED\x8F\x90\xEA\xB8\xB0") return "OP" + suffix;  // 개폐기
  }
  if (type == "RELAY") return "R"  + String(deviceId % 100);
  if (type == "MOTOR") return "M"  + String(deviceId % 100);
  return "D" + String(deviceId % 100);
}

namespace {
unsigned long g_lastPollMs   = 0;
String        g_lastError;
int           g_lastCount    = 0;
bool          g_firstPoll    = true;
}

namespace ServerPollBridge {

void begin() {
  g_lastPollMs = 0;
  g_lastError  = "";
  g_lastCount  = 0;
  g_firstPoll  = true;
}

void loop() {
  if (!WifiManager::ready()) return;
  if (!g_firstPoll && millis() - g_lastPollMs < SERVER_POLL_INTERVAL_MS) return;

  g_lastPollMs = millis();
  if (g_firstPoll) AudioManager::speak(VoiceEvent::Query);

  // --- HTTP GET ---
  HTTPClient http;
  String url = makeServerUrl(API_ACTUATOR_LIST_GET);
  url += "?farmIndex=" + String(KOAT_FARM_INDEX);

  Serial.println("[POLL] GET " + url);
  http.begin(url);
  http.addHeader("AppKey", APP_KEY);
  http.setTimeout(HTTP_TIMEOUT_MS);

  const int code = http.GET();
  String body;
  if (code == HTTP_CODE_OK) body = http.getString();
  http.end();

  if (code != HTTP_CODE_OK) {
    g_lastError = "HTTP " + String(code);
    Serial.println("[POLL] Error: " + g_lastError);
    AudioManager::speak(VoiceEvent::Fail);
    return;
  }
  Serial.printf("[POLL] %d bytes received\n", body.length());

  // --- JSON parse ---
  // Supports three shapes:
  //   [ {...}, ... ]
  //   {"data":   [ {...}, ... ]}
  //   {"actuators": [ {...}, ... ]}
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, body);
  if (err) {
    g_lastError = String("JSON:") + err.c_str();
    Serial.println("[POLL] " + g_lastError);
    AudioManager::speak(VoiceEvent::Fail);
    return;
  }

  JsonArray arr;
  if      (doc.is<JsonArray>())             arr = doc.as<JsonArray>();
  else if (doc["data"].is<JsonArray>())     arr = doc["data"].as<JsonArray>();
  else if (doc["actuators"].is<JsonArray>())arr = doc["actuators"].as<JsonArray>();
  else {
    g_lastError = "unexpected JSON shape";
    Serial.println("[POLL] " + g_lastError);
    AudioManager::speak(VoiceEvent::Fail);
    return;
  }

  // --- Apply to registry ---
  g_lastCount   = 0;
  bool anyChange = false;

  for (JsonObject obj : arr) {
    // Accept either "id" or "serial" as device identifier
    const int id = obj["id"].is<JsonInteger>()     ? (int)obj["id"].as<JsonInteger>()
                 : obj["serial"].is<JsonInteger>() ? (int)obj["serial"].as<JsonInteger>()
                 : 0;
    if (id <= 0) continue;

    const String name       = obj["name"]       | "";
    const String zone       = obj["zone"]       | "";
    const String type       = obj["type"]       | "";
    const String srvStatus  = obj["status"]     | "normal";
    const uint16_t command  = obj["command"].is<JsonInteger>()   ? (uint16_t)(int)obj["command"].as<JsonInteger>()   : 0;
    const uint32_t remain   = obj["remainSec"].is<JsonInteger>() ? (uint32_t)(int)obj["remainSec"].as<JsonInteger>() : 0;

    const String label = generateLabel(name, type, id);
    anyChange |= ActuatorRegistry::applyFromServer(id, label, name, zone, type, srvStatus, command, remain);
    ++g_lastCount;
    Serial.printf("[POLL]   id=%d label=%s cmd=%u remain=%lu\n", id, label.c_str(), (unsigned)command, (unsigned long)remain);
  }

  if (anyChange) AudioManager::speak(VoiceEvent::Update);
  g_lastError = "";
  g_firstPoll = false;
  Serial.printf("[POLL] Done: %d actuators, changed=%d\n", g_lastCount, anyChange ? 1 : 0);
}

String lastError()       { return g_lastError; }
int    lastActuatorCount(){ return g_lastCount;  }

}
