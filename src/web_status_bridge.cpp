#include "web_status_bridge.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "app_config.h"

namespace {
static String updateURL = makeServerUrl(API_DEVICE_STATUS_POST);
static String deviceStandardStatusURL = makeServerUrl(API_DEVICE_STANDARD_STATUS_POST);
static String deviceBoardStatusURL = makeServerUrl(API_DEVICE_BOARD_STATUS_POST);

static uint16_t sanitizeBoardStatus(uint16_t status) {
  return (status <= 6) ? status : 1;
}
}

namespace WebStatusBridge {

void begin() {}

bool updateDeviceStatus(int deviceId, int actValue, int statusDevice) {
  if (WiFi.status() != WL_CONNECTED || deviceId <= 0) return false;
  HTTPClient http;
  http.begin(updateURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("AppKey", APP_KEY);
  String body = "id=" + String(deviceId) + "&valueAct=" + String(actValue) + "&deviceStatus_id=" + String(statusDevice);
  Serial.printf("[WEB] POST %s\n", updateURL.c_str());
  Serial.printf("[WEB] body: %s\n", body.c_str());
  int code = http.POST(body);
  if (code > 0) {
    const String payload = http.getString();
    if (payload.length()) Serial.println(payload);
  }
  http.end();
  return code >= 200 && code < 300;
}

bool postDeviceStandardStatusToServer(int deviceId, uint16_t statusIndex) {
  if (WiFi.status() != WL_CONNECTED || deviceId <= 0) return false;
  HTTPClient http;
  http.begin(deviceStandardStatusURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("AppKey", APP_KEY);
  String body = "id=" + String(deviceId) + "&status_index=" + String(statusIndex);
  Serial.printf("[DEVICE-STD] POST %s\n", deviceStandardStatusURL.c_str());
  Serial.printf("[DEVICE-STD] body: %s\n", body.c_str());
  int code = http.POST(body);
  if (code > 0) {
    const String payload = http.getString();
    if (payload.length()) Serial.println(payload);
  }
  http.end();
  return code >= 200 && code < 300;
}

bool postDeviceBoardStatusToServer(int farmIndex, uint16_t statusIndex) {
  if (WiFi.status() != WL_CONNECTED || farmIndex <= 0) return false;
  HTTPClient http;
  http.begin(deviceBoardStatusURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("AppKey", APP_KEY);
  String body = "farmIndex=" + String(farmIndex) + "&status_index=" + String(sanitizeBoardStatus(statusIndex));
  int code = http.POST(body);
  if (code > 0) {
    const String payload = http.getString();
    if (payload.length()) Serial.println(payload);
  }
  http.end();
  return code >= 200 && code < 300;
}

bool reportActuatorStatusToServer(int deviceId, int state) {
  return updateDeviceStatus(deviceId, 0, state == 0 ? 0 : 1);
}

}
