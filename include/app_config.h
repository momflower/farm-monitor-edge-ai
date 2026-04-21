#pragma once
#include <Arduino.h>

// ===== Wi-Fi =====
static constexpr const char* WIFI_SSID = "KT_GiGA_D665";
static constexpr const char* WIFI_PASSWORD = "cage3cb196";

// ===== Server =====
#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS "http://119.207.62.14/spring"
#endif

static constexpr const char* APP_KEY       = "f54aa895-0aca-4084-9b10-aecaff878af1";
static constexpr int          KOAT_FARM_INDEX = 20;

// Legacy reporting endpoints (MCU → server)
static constexpr const char* API_DEVICE_STATUS_POST          = "/admin/device/updateMCUBoardPOST";
static constexpr const char* API_DEVICE_STANDARD_STATUS_POST = "/admin/device/updateMCUBoardPOST/StandardStatus";
static constexpr const char* API_DEVICE_BOARD_STATUS_POST    = "/admin/device/updateMCUBoardPOST/BoardStatus";

// Actuator list polling endpoint (server → MCU)
// GET {SERVER_ADDRESS}{API_ACTUATOR_LIST_GET}?farmIndex={KOAT_FARM_INDEX}
// Expected JSON: array or {"data":[...]} of actuator objects with fields:
//   id(int), name(str), zone(str), type(str), status(str), command(int), remainSec(int)
static constexpr const char* API_ACTUATOR_LIST_GET = "/admin/device/list";

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

// ===== Display (1.85" circle GC9A01, 360×360) =====
static constexpr int LCD_SPI_SCLK = 12;
static constexpr int LCD_SPI_MOSI = 11;
static constexpr int LCD_SPI_MISO = -1;
static constexpr int LCD_SPI_CS   = 10;
static constexpr int LCD_SPI_DC   = 8;
static constexpr int LCD_RST      = 14;
static constexpr int LCD_BL       = 9;
static constexpr int LCD_WIDTH    = 360;
static constexpr int LCD_HEIGHT   = 360;
static constexpr int LCD_ROTATION = 0;

// ===== Audio =====
static constexpr bool ENABLE_VOICE_PLAYBACK = false;
static constexpr bool ENABLE_BEEP_FALLBACK  = true;
static constexpr int  BUZZER_PIN            = -1;

static inline String makeServerUrl(const char* path) {
  return String(SERVER_ADDRESS) + String(path);
}
