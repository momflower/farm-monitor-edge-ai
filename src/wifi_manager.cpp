#include "wifi_manager.h"
#include <WiFi.h>
#include "app_config.h"

namespace {
unsigned long g_lastAttempt = 0;
}

namespace WifiManager {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  g_lastAttempt = millis();
  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - g_lastAttempt < WIFI_RETRY_INTERVAL_MS) return;
  g_lastAttempt = millis();
  Serial.println("[WiFi] retry");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool ready() { return WiFi.status() == WL_CONNECTED; }
String ip() { return ready() ? WiFi.localIP().toString() : String("0.0.0.0"); }

}
