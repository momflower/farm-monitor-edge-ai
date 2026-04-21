import os

Import("env")

# The installed moononournation/GFX Library for Arduino 1.6.5 in this project
# ships a modified Arduino_ESP32RGBPanel.h whose guard for the private
# esp_rgb_panel_t struct was changed from "< 3" to "> 5". The matching .cpp
# still uses "< 3", so on arduino-esp32 2.x (ESP_ARDUINO_VERSION_MAJOR=2)
# the struct is never defined and compilation fails.
# We patch the header back to the correct "< 3" conditional. We don't use
# the RGB panel at all (the project uses Arduino_ESP32QSPI), so getting the
# file to compile is sufficient.

TARGET = os.path.join(
    env["PROJECT_LIBDEPS_DIR"],
    env["PIOENV"],
    "GFX Library for Arduino",
    "src",
    "databus",
    "Arduino_ESP32RGBPanel.h",
)

BROKEN = "#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR >5)"
FIXED  = "#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)"

if os.path.exists(TARGET):
    with open(TARGET, "r", encoding="utf-8") as f:
        content = f.read()
    if BROKEN in content:
        content = content.replace(BROKEN, FIXED)
        with open(TARGET, "w", encoding="utf-8") as f:
            f.write(content)
        print(">> patch_gfx_library: fixed esp_rgb_panel_t guard in Arduino_ESP32RGBPanel.h")
