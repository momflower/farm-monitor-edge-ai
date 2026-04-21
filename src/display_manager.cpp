#include "display_manager.h"
#include <LovyanGFX.hpp>
#include "app_config.h"
#include "actuator_registry.h"

// ─── LovyanGFX driver for GC9A01 (1.85" circle, 360×360) ────────────────────
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;
public:
  LGFX() {
    {
      auto cfg      = _bus.config();
      cfg.spi_host  = SPI2_HOST;
      cfg.spi_mode  = 0;
      cfg.freq_write= 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock  = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk  = LCD_SPI_SCLK;
      cfg.pin_mosi  = LCD_SPI_MOSI;
      cfg.pin_miso  = LCD_SPI_MISO;
      cfg.pin_dc    = LCD_SPI_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg          = _panel.config();
      cfg.pin_cs        = LCD_SPI_CS;
      cfg.pin_rst       = LCD_RST;
      cfg.pin_busy      = -1;
      cfg.memory_width  = LCD_WIDTH;
      cfg.memory_height = LCD_HEIGHT;
      cfg.panel_width   = LCD_WIDTH;
      cfg.panel_height  = LCD_HEIGHT;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      cfg.offset_rotation = 0;
      cfg.readable      = false;
      cfg.invert        = false;
      cfg.rgb_order     = false;
      cfg.dlen_16bit    = false;
      cfg.bus_shared    = true;
      _panel.config(cfg);
    }
    {
      auto cfg       = _light.config();
      cfg.pin_bl     = LCD_BL;
      cfg.invert     = false;
      cfg.freq       = 44100;
      cfg.pwm_channel= 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
namespace {
LGFX tft;

void drawCentered(const String& text, int y, int sz, uint16_t color = TFT_WHITE) {
  tft.setTextDatum(middle_center);
  tft.setTextSize(sz);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawCenterString(text, tft.width() / 2, y);
}

// 4-char max status labels for compact 2-column layout
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
  if (!online) return TFT_RED;
  if (s == "ON" || s == "OPEN" || s == "T-ON" || s == "T-OP") return TFT_GREEN;
  if (s == "OFF" || s == "CLSE" || s == "T-CL")               return TFT_ORANGE;
  return TFT_YELLOW;
}

// Draw one actuator cell inside the 2-column grid.
// (cx, cy) = top-left origin of this cell; cw = cell width.
void drawCell(const ActuatorState& s, int cx, int cy, int cw) {
  const String stat  = s.online ? compactStatus(s.command) : "---";
  const uint16_t col = statusColor(stat, s.online);

  // Row 1: label (left) + status (right)  — size 2
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(top_left);
  tft.drawString(s.label, cx, cy);

  tft.setTextColor(col, TFT_BLACK);
  tft.setTextDatum(top_right);
  tft.drawString(stat, cx + cw, cy);

  // Row 2: type icon (left) + remaining time (right)  — size 1
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(top_left);
  if (!s.type.isEmpty()) tft.drawString(s.type.substring(0, 1), cx, cy + 20);
  if (s.online && s.remainSec > 0) {
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextDatum(top_right);
    tft.drawString(String(s.remainSec) + "s", cx + cw, cy + 20);
  }
}
}  // namespace

// ─── Public API ──────────────────────────────────────────────────────────────
namespace DisplayManager {

void begin() {
  tft.init();
  tft.setRotation(LCD_ROTATION);
  tft.setBrightness(180);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawBoot() {
  tft.fillScreen(TFT_BLACK);
  drawCentered("FARM", 120, 3, TFT_CYAN);
  drawCentered("MONITOR", 160, 3, TFT_CYAN);
  drawCentered("Booting...", 210, 2);
}

void drawPolling(const String& ip) {
  tft.fillScreen(TFT_BLACK);
  drawCentered("FARM MONITOR", 120, 2, TFT_CYAN);
  drawCentered("Polling...", 162, 2, TFT_YELLOW);
  drawCentered(ip, 210, 1, TFT_LIGHTGREY);
}

void drawError(const String& msg) {
  tft.fillScreen(TFT_BLACK);
  drawCentered("ERROR", 110, 3, TFT_RED);
  drawCentered(msg, 168, 2);
}

// 2-column grid layout for up to MAX_DISPLAY_ACTUATORS (10) actuators.
//
// Circle 360×360, centre (180,180), radius 180.
// Safe x at y=52 (topmost content row, 128px from centre):
//   dx = sqrt(180²−128²) ≈ 130 → safe x ≈ 50 … 310.
//
// Layout:
//   y= 18  header "FARM MONITOR"  (size 2, CYAN)
//   y= 38  top divider
//   y= 52  Row 0 … Row 4  (row height 44px, 2 cells per row)
//   y=272  bottom divider
//   y=284  footer
//
//   Left  cell: label x=55,  status right-align x=170
//   Right cell: label x=192, status right-align x=307
void drawStates(const ActuatorState* states, size_t count, const String& footer) {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.drawCenterString("FARM MONITOR", tft.width() / 2, 18);
  tft.drawFastHLine(30, 35, tft.width() - 60, TFT_DARKGREY);

  if (count == 0) {
    drawCentered("No actuators", 180, 2, TFT_YELLOW);
  } else {
    // Two-column grid: left col x=[55..170], right col x=[192..307]
    constexpr int ROW_H    = 44;   // height per row
    constexpr int Y_START  = 52;   // first row top
    constexpr int COL_L_X  = 55;   // left col label origin
    constexpr int COL_L_RX = 170;  // left col status right edge
    constexpr int COL_R_X  = 192;  // right col label origin
    constexpr int COL_R_RX = 307;  // right col status right edge
    constexpr int COL_W_L  = COL_L_RX - COL_L_X;  // 115
    constexpr int COL_W_R  = COL_R_RX - COL_R_X;  // 115

    const size_t shown = (count < MAX_DISPLAY_ACTUATORS) ? count : MAX_DISPLAY_ACTUATORS;
    for (size_t i = 0; i < shown; ++i) {
      const int row = i / 2;
      const int col = i % 2;
      const int cy  = Y_START + row * ROW_H;
      if (col == 0) {
        drawCell(states[i], COL_L_X, cy, COL_W_L);
      } else {
        // vertical separator dot at x=181
        tft.drawFastVLine(181, cy, ROW_H - 6, TFT_DARKGREY);
        drawCell(states[i], COL_R_X, cy, COL_W_R);
      }
    }

    // Overflow indicator
    if (count > MAX_DISPLAY_ACTUATORS) {
      const int overflowY = Y_START + (MAX_DISPLAY_ACTUATORS / 2) * ROW_H;
      tft.setTextSize(1);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextDatum(top_right);
      tft.drawString("+" + String(count - MAX_DISPLAY_ACTUATORS) + " more", tft.width() - 30, overflowY);
    }
  }

  // Footer
  tft.drawFastHLine(30, 272, tft.width() - 60, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.drawCenterString(footer, tft.width() / 2, 284);
}

}  // namespace DisplayManager
