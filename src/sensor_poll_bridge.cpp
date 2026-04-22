#include "sensor_poll_bridge.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "app_config.h"

namespace {

// Exact sensorName strings as returned by GET /admin/edge/farm/sensor/{id}.
// Stored as UTF-8 byte literals so we don't depend on the compiler encoding.
// 온도1 = EC 98 A8 EB 8F 84 31
// 온도2 = EC 98 A8 EB 8F 84 32
// 습도1 = EC 8A B5 EB 8F 84 31
// 습도2 = EC 8A B5 EB 8F 84 32
// 감우  = EA B0 90 EC 9A B0
// 유량  = EC 9C A0 EB 9F 89
constexpr const char* NAME_TEMP1 = "\xEC\x98\xA8\xEB\x8F\x84\x31";
constexpr const char* NAME_TEMP2 = "\xEC\x98\xA8\xEB\x8F\x84\x32";
constexpr const char* NAME_HUM1  = "\xEC\x8A\xB5\xEB\x8F\x84\x31";
constexpr const char* NAME_HUM2  = "\xEC\x8A\xB5\xEB\x8F\x84\x32";
constexpr const char* NAME_RAIN  = "\xEA\xB0\x90\xEC\x9A\xB0";
constexpr const char* NAME_FLOW  = "\xEC\x9C\xA0\xEB\x9F\x89";

void assignByName(SensorSnapshot& s, const char* name, float value) {
  if      (strcmp(name, NAME_TEMP1) == 0) s.temp1 = value;
  else if (strcmp(name, NAME_TEMP2) == 0) s.temp2 = value;
  else if (strcmp(name, NAME_HUM1)  == 0) s.hum1  = value;
  else if (strcmp(name, NAME_HUM2)  == 0) s.hum2  = value;
  else if (strcmp(name, NAME_RAIN)  == 0) s.rain  = value;
  else if (strcmp(name, NAME_FLOW)  == 0) s.flow  = value;
}

}  // namespace

namespace SensorPollBridge {

SensorSnapshot fetchOnce() {
  SensorSnapshot snap;

  HTTPClient http;
  const String url = makeEdgeUrl(API_EDGE_FARM_SENSOR_GET_BASE) + String(EDGE_FARM_INDEX);

  Serial.println("[SENSOR] GET " + url);

  http.begin(url);
  http.addHeader("AppKey", APP_KEY);
  http.setTimeout(HTTP_TIMEOUT_MS);

  const int code = http.GET();
  String payload;
  if (code > 0) payload = http.getString();
  http.end();

  Serial.printf("[SENSOR] HTTP %d, %u bytes\n", code, (unsigned)payload.length());
  if (payload.length() > 0 && payload.length() < 4096) {
    Serial.println("[SENSOR] payload:");
    Serial.println(payload);
  }

  if (code != HTTP_CODE_OK) {
    snap.error = String("HTTP ") + code;
    return snap;
  }

  JsonDocument doc;
  const DeserializationError jerr = deserializeJson(doc, payload);
  if (jerr) {
    snap.error = String("JSON:") + jerr.c_str();
    Serial.println("[SENSOR] " + snap.error);
    return snap;
  }

  if (!doc.is<JsonArrayConst>()) {
    snap.error = "not a JSON array";
    Serial.println("[SENSOR] " + snap.error);
    return snap;
  }

  JsonArrayConst arr = doc.as<JsonArrayConst>();
  for (JsonObjectConst obj : arr) {
    const char* name = obj["sensorName"] | (const char*)nullptr;
    if (!name) continue;
    const float value = obj["value"].is<float>() ? obj["value"].as<float>()
                                                 : obj["value"].as<int>();
    assignByName(snap, name, value);
  }

  snap.valid = !isnan(snap.temp1) || !isnan(snap.temp2) ||
               !isnan(snap.hum1)  || !isnan(snap.hum2)  ||
               !isnan(snap.rain)  || !isnan(snap.flow);
  if (!snap.valid && snap.error.length() == 0) {
    snap.error = arr.size() == 0 ? "empty sensor list"
                                 : "no target sensors found";
  }

  Serial.printf("[SENSOR] parsed: T1=%.1f T2=%.1f H1=%.1f H2=%.1f Rain=%.1f Flow=%.2f valid=%d\n",
                snap.temp1, snap.temp2, snap.hum1, snap.hum2,
                snap.rain,  snap.flow,  snap.valid ? 1 : 0);
  return snap;
}

}  // namespace SensorPollBridge
