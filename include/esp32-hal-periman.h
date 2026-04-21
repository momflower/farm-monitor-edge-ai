#pragma once
// Compatibility stub — Arduino_GFX SPI bus driver needs this header but
// the installed framework-arduinoespressif32 v3.x does not provide it.
// We only use Arduino_ESP32QSPI, so these SPI-periman stubs are never called.
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ESP32_BUS_TYPE_INIT           = 0,
  ESP32_BUS_TYPE_GPIO,
  ESP32_BUS_TYPE_UART,
  ESP32_BUS_TYPE_SPI_MASTER_SCK,
  ESP32_BUS_TYPE_SPI_MASTER_MOSI,
  ESP32_BUS_TYPE_SPI_MASTER_MISO,
  ESP32_BUS_TYPE_SPI_MASTER_SS,
  ESP32_BUS_TYPE_SPI_SLAVE_SCK,
  ESP32_BUS_TYPE_SPI_SLAVE_MOSI,
  ESP32_BUS_TYPE_SPI_SLAVE_MISO,
  ESP32_BUS_TYPE_SPI_SLAVE_SS,
  ESP32_BUS_TYPE_I2C_MASTER_SDA,
  ESP32_BUS_TYPE_I2C_MASTER_SCL,
  ESP32_BUS_TYPE_I2C_SLAVE_SDA,
  ESP32_BUS_TYPE_I2C_SLAVE_SCL,
  ESP32_BUS_TYPE_I2S,
  ESP32_BUS_TYPE_LEDC,
  ESP32_BUS_TYPE_SIGMADELTA,
  ESP32_BUS_TYPE_RMT,
  ESP32_BUS_TYPE_ADC_CONT,
  ESP32_BUS_TYPE_ADC_ONESHOT,
  ESP32_BUS_TYPE_TOUCH,
  ESP32_BUS_TYPE_ANALOG,
  ESP32_BUS_TYPE_MAX
} peripheral_bus_type_t;

static inline bool  perimanSetPinBus(uint8_t p, peripheral_bus_type_t t, void *b, int8_t ch, int8_t addr) { (void)p;(void)t;(void)b;(void)ch;(void)addr; return true; }
static inline void* perimanGetPinBus(uint8_t p, peripheral_bus_type_t t)  { (void)p;(void)t; return NULL; }
static inline bool  perimanClearPinBus(uint8_t p)                          { (void)p; return true; }

#ifdef __cplusplus
}
#endif
