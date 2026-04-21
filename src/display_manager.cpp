#include "display_manager.h"
#include <Arduino_GFX_Library.h>
#include "app_config.h"
#include "actuator_registry.h"

// ─── Arduino_GFX driver for ST77916 (1.85" QSPI circle, 360×360) ────────────
namespace {

Arduino_DataBus *_bus = new Arduino_ESP32QSPI(
    LCD_QSPI_CS,  LCD_QSPI_CLK,
    LCD_QSPI_D0,  LCD_QSPI_D1,
    LCD_QSPI_D2,  LCD_QSPI_D3,
    20000000 /*20 MHz — lower speed reduces QSPI timing artifacts*/);

// 150-series init uses WRITE_COMMAND_8 for SLPOUT/DISPON/INVON (correct QSPI
// format). IPS=true so the library confirms inversion state after init.
// tftInit() is patched to not send a second invertDisplay, keeping init clean.
Arduino_GFX *gfx = new Arduino_ST77916(
    _bus, GFX_NOT_DEFINED, 0 /*rotation*/, true /*IPS*/,
    360, 360, 0, 0, 0, 0,
    st77916_150_init_operations, sizeof(st77916_150_init_operations));

// RGB565 color constants (explicit — Arduino_GFX does not export Adafruit names)
constexpr uint16_t C_BLACK   = 0x0000;
constexpr uint16_t C_WHITE   = 0xFFFF;
constexpr uint16_t C_CYAN    = 0x07FF;
constexpr uint16_t C_YELLOW  = 0xFFE0;
constexpr uint16_t C_RED     = 0xF800;
constexpr uint16_t C_GREEN   = 0x07E0;
constexpr uint16_t C_ORANGE  = 0xFD20;
constexpr uint16_t C_DKGREY  = 0x7BEF;
constexpr uint16_t C_LTGREY  = 0xC618;

// Pixel width of a string using the built-in 5×7 font at scale sz
inline int tw(const String& s, int sz) { return (int)s.length() * 6 * sz; }
inline int tw(const char* s, int sz)   { return (int)strlen(s)  * 6 * sz; }

// Draw text horizontally centered, vertically centred at cy
void drawCentered(const String& text, int cy, int sz, uint16_t color = 0xFFFF) {
  gfx->setTextSize(sz);
  gfx->setTextColor(color, C_BLACK);
  gfx->setCursor((LCD_WIDTH - tw(text, sz)) / 2, cy - 4 * sz);
  gfx->print(text);
}

// 4-char max status label
String compactStatus(uint16_t cmd) {
  switch (cmd) {
    case KsxCommand::STOP:        return "OFF";
    case KsxCommand::SWITCH_ON:   return "ON";
    case KsxCommand::SWITCH_TIME: return "T-ON";
    case KsxCommand::OPEN_ON:     return "OPEN";
    case KsxCommand::CLOSE_ON:    return "CLSE";
    case KsxCommand::OPEN_TIME:   return "T-OP";
    case KsxCommand::CLOSE_TIME:  return "T-CL";
    default:                      return String(cmd);
  }
}

uint16_t statusColor(const String& s, bool online) {
  if (!online) return C_RED;
  if (s == "ON" || s == "OPEN" || s == "T-ON" || s == "T-OP") return C_GREEN;
  if (s == "OFF" || s == "CLSE" || s == "T-CL")               return C_ORANGE;
  return C_YELLOW;
}

// Draw one actuator cell in the 2-column grid.
// (cx, cy) = top-left of cell; cw = cell width.
void drawCell(const ActuatorState& s, int cx, int cy, int cw) {
  const String stat  = s.online ? compactStatus(s.command) : "---";
  const uint16_t col = statusColor(stat, s.online);

  // Row 1: label (left) + status (right) — size 2
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE, C_BLACK);
  gfx->setCursor(cx, cy);
  gfx->print(s.label);

  gfx->setTextColor(col, C_BLACK);
  gfx->setCursor(cx + cw - tw(stat, 2), cy);
  gfx->print(stat);

  // Row 2: type initial (left) + remaining time (right) — size 1
  gfx->setTextSize(1);
  if (!s.type.isEmpty()) {
    gfx->setTextColor(C_DKGREY, C_BLACK);
    gfx->setCursor(cx, cy + 20);
    gfx->print(s.type.charAt(0));
  }
  if (s.online && s.remainSec > 0) {
    String rem = String(s.remainSec) + "s";
    gfx->setTextColor(C_LTGREY, C_BLACK);
    gfx->setCursor(cx + cw - tw(rem, 1), cy + 20);
    gfx->print(rem);
  }
}

}  // namespace

// ─── Public API ──────────────────────────────────────────────────────────────
namespace DisplayManager {

void begin() {
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  gfx->begin();
  gfx->setRotation(LCD_ROTATION);
  delay(50);
  gfx->fillScreen(C_BLACK);
}

void drawBoot() {
  gfx->fillScreen(C_BLACK);
  drawCentered("SLOWER",    130, 3, C_CYAN);
  drawCentered("MONITOR",   170, 3, C_CYAN);
  drawCentered("Booting...",210, 2, C_WHITE);
}

void drawPolling(const String& ip) {
  gfx->fillScreen(C_BLACK);
  drawCentered("SLOWER MONITOR", 120, 2, C_CYAN);
  drawCentered("Polling...",   162, 2, C_YELLOW);
  drawCentered(ip,             210, 1, C_LTGREY);
}

void drawError(const String& msg) {
  gfx->fillScreen(C_BLACK);
  drawCentered("ERROR", 110, 3, C_RED);
  drawCentered(msg,     168, 2, C_WHITE);
}

// 2-column grid:
//   y=28   header "SLOWER MONITOR"  (size 2, CYAN)
//   y=45   top divider  (+10px from original)
//   y=62   Row 0 … Row 4  (44 px each, 2 cells per row)
//   y=272  bottom divider
//   y=284  footer (size 1)
//
//   Left  cell: label x=55,  status right-edge x=170  (cw=115)
//   Right cell: label x=192, status right-edge x=307  (cw=115)
void drawStates(const ActuatorState* states, size_t count, const String& footer) {
  gfx->fillScreen(C_BLACK);

  // Header (y=28 → 10px lower than the original y=18)
  gfx->setTextSize(2);
  gfx->setTextColor(C_CYAN, C_BLACK);
  const char* hdr = "SLOWER MONITOR";
  gfx->setCursor((LCD_WIDTH - tw(hdr, 2)) / 2, 28 - 8);
  gfx->print(hdr);
  gfx->drawFastHLine(30, 45, LCD_WIDTH - 60, C_DKGREY);

  if (count == 0) {
    drawCentered("No actuators", 180, 2, C_YELLOW);
  } else {
    constexpr int ROW_H    = 44;
    constexpr int Y_START  = 62;  // +10px: header/divider shifted down
    constexpr int COL_L_X  = 55;
    constexpr int COL_L_RX = 170;
    constexpr int COL_R_X  = 192;
    constexpr int COL_R_RX = 307;
    constexpr int COL_W_L  = COL_L_RX - COL_L_X;
    constexpr int COL_W_R  = COL_R_RX - COL_R_X;

    const size_t shown = (count < MAX_DISPLAY_ACTUATORS) ? count : MAX_DISPLAY_ACTUATORS;
    for (size_t i = 0; i < shown; ++i) {
      const int row = i / 2;
      const int col = i % 2;
      const int cy  = Y_START + row * ROW_H;
      if (col == 0) {
        drawCell(states[i], COL_L_X, cy, COL_W_L);
      } else {
        gfx->drawFastVLine(181, cy, ROW_H - 6, C_DKGREY);
        drawCell(states[i], COL_R_X, cy, COL_W_R);
      }
    }

    if (count > MAX_DISPLAY_ACTUATORS) {
      String more = "+" + String(count - MAX_DISPLAY_ACTUATORS) + " more";
      gfx->setTextSize(1);
      gfx->setTextColor(C_YELLOW, C_BLACK);
      gfx->setCursor(LCD_WIDTH - 30 - tw(more, 1),
                     Y_START + (MAX_DISPLAY_ACTUATORS / 2) * ROW_H);
      gfx->print(more);
    }
  }

  // Footer
  gfx->drawFastHLine(30, 272, LCD_WIDTH - 60, C_DKGREY);
  gfx->setTextSize(1);
  gfx->setTextColor(C_LTGREY, C_BLACK);
  gfx->setCursor((LCD_WIDTH - tw(footer, 1)) / 2, 284 - 4);
  gfx->print(footer);
}

}  // namespace DisplayManager
