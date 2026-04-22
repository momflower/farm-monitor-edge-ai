#pragma once
#include <Arduino.h>

// ===== Wi-Fi =====
static constexpr const char* WIFI_SSID = "mfg-garden";
static constexpr const char* WIFI_PASSWORD = "garden2966!";

// ===== Server =====
#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS "http://119.207.62.14/spring"
#endif
// Edge services (farm info / sensor / reservation) are on port 8080.
#ifndef EDGE_SERVER_ADDRESS
#define EDGE_SERVER_ADDRESS "http://119.207.62.14:8080"
#endif

static constexpr const char* APP_KEY       = "f54aa895-0aca-4084-9b10-aecaff878af1";
static constexpr int          KOAT_FARM_INDEX = 20;
// Edge services (port 8080) resolve by the *numeric* farm id — the dashboard's
// short-id like "JxjXno6X" is a frontend-only alias that maps to this integer.
// NOTE: farm 4 currently has no sensor data; using 20 which has full readings.
static constexpr int          EDGE_FARM_INDEX = 20;

// Actuator list query endpoint (server → MCU, POST)
// POST body: farmIndex={KOAT_FARM_INDEX}
// Returns JSON array or {"data":[...]} of actuator objects:
//   id(int), name(str), zone(str), type(str), status(str), command(int), remainSec(int)
static constexpr const char* API_DEVICE_COMMAND_POST = "/admin/device/selectAreaMCUBoardPOST";

// Edge services (port 8080). Probed schema:
//   GET {base}/admin/edge/farm/sensor/{EDGE_FARM_INDEX}          → JSON array
//   GET {base}/admin/edge/farm/reservation/list/{EDGE_FARM_INDEX} → JSON array
//   POST {base}/admin/edge/farm/{EDGE_FARM_INDEX}                 → farm info object
//   headers: AppKey (required)
// The farmIndex is the *numeric* farm id (e.g. 4), not the dashboard slug.
static constexpr const char* API_EDGE_FARM_SENSOR_GET_BASE      = "/admin/edge/farm/sensor/";
static constexpr const char* API_EDGE_FARM_RESERVATION_GET_BASE = "/admin/edge/farm/reservation/list/";
static constexpr const char* API_EDGE_FARM_INFO_POST_BASE       = "/admin/edge/farm/";

// Sensor boot-splash duration before handing over to actuator monitoring.
static constexpr uint32_t SENSOR_SPLASH_MS = 5000;

// Legacy reporting endpoints (MCU → server)
static constexpr const char* API_DEVICE_STATUS_POST          = "/admin/device/updateMCUBoardPOST";
static constexpr const char* API_DEVICE_STANDARD_STATUS_POST = "/admin/device/updateMCUBoardPOST/StandardStatus";
static constexpr const char* API_DEVICE_BOARD_STATUS_POST    = "/admin/device/updateMCUBoardPOST/BoardStatus";

// ===== Timing =====
static constexpr uint32_t WIFI_RETRY_INTERVAL_MS  = 5000;
static constexpr uint32_t UI_REFRESH_INTERVAL_MS  = 250;
static constexpr uint32_t VOICE_MIN_INTERVAL_MS   = 1200;
static constexpr uint32_t SERVER_POLL_INTERVAL_MS = 5000;   // how often to poll actuator list
static constexpr uint32_t STALE_OFFLINE_MS        = 30000;  // mark offline after 30s without update
static constexpr int      HTTP_TIMEOUT_MS         = 8000;

// ===== Registry limits =====
static constexpr size_t MAX_ACTUATOR_REGISTRY_SIZE = 16;  // max tracked actuators
static constexpr size_t MAX_DISPLAY_ACTUATORS      = 10;  // max shown on LCD (2col × 5rows)

// ===== KSX command/status =====
namespace KsxCommand {
  constexpr uint16_t STOP       = 0;
  constexpr uint16_t SWITCH_ON  = 201;
  constexpr uint16_t SWITCH_TIME= 202;
  constexpr uint16_t OPEN_ON    = 301;
  constexpr uint16_t CLOSE_ON   = 302;
  constexpr uint16_t OPEN_TIME  = 303;
  constexpr uint16_t CLOSE_TIME = 304;
}

// ===== Seed actuators (used as initial label map; server response populates the full list) =====
struct MonitoredActuatorConfig {
  int deviceId;
  const char* label;
};

static constexpr MonitoredActuatorConfig MONITORED_ACTUATORS[] = {
  {222, "SW1"}, {223, "SW2"}, {224, "SW3"}, {225, "SW4"}, {226, "SW5"}, {227, "SW6"},
  {228, "OP1"}, {229, "OP2"}, {230, "OP3"},
};
static constexpr size_t MONITORED_ACTUATOR_COUNT = sizeof(MONITORED_ACTUATORS) / sizeof(MONITORED_ACTUATORS[0]);

// ===== Display — ESP32-S3-Touch-LCD-1.85C, ST77916 QSPI, 360×360 =====
static constexpr int LCD_QSPI_CLK  = 40;
static constexpr int LCD_QSPI_CS   = 21;
static constexpr int LCD_QSPI_D0   = 46;
static constexpr int LCD_QSPI_D1   = 45;
static constexpr int LCD_QSPI_D2   = 42;
static constexpr int LCD_QSPI_D3   = 41;
static constexpr int LCD_BL        = 5;
static constexpr int LCD_WIDTH     = 360;
static constexpr int LCD_HEIGHT    = 360;
static constexpr int LCD_ROTATION  = 0;

// ===== Audio — ES8311 codec + NS4150B amp (V2 board) =====
// Waveshare ESP32-S3-Touch-LCD-1.85C (Speaker Box V2, No Batt)
static constexpr int AUDIO_I2S_MCLK = 2;    // MCLK → ES8311
static constexpr int AUDIO_I2S_BCLK = 48;   // BCK
static constexpr int AUDIO_I2S_WS   = 38;   // LRCK
static constexpr int AUDIO_I2S_DOUT = 47;   // DOUT → ES8311 SDIN
static constexpr int AUDIO_PA_CTRL  = 15;   // NS4150B amp enable (HIGH = on)
static constexpr int ES8311_SDA     = 11;   // I2C SDA to ES8311
static constexpr int ES8311_SCL     = 10;   // I2C SCL to ES8311
static constexpr uint8_t ES8311_ADDR = 0x18;

static inline String makeServerUrl(const char* path) {
  return String(SERVER_ADDRESS) + String(path);
}

static inline String makeEdgeUrl(const char* path) {
  return String(EDGE_SERVER_ADDRESS) + String(path);
}
