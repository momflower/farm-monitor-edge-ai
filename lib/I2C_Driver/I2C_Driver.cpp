#include "I2C_Driver.h"
#include <Wire.h>

static constexpr uint8_t I2C_SDA_PIN = 11;
static constexpr uint8_t I2C_SCL_PIN = 10;
static constexpr uint32_t I2C_FREQ_HZ = 400000;

extern "C" void I2C_Init(void) {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
  delay(10);
}

extern "C" bool i2c_reg8_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, size_t len) {
  Wire.beginTransmission(dev_addr);
  Wire.write(reg_addr);
  for (size_t i = 0; i < len; ++i) {
    Wire.write(data[i]);
  }
  return Wire.endTransmission() == 0;
}

extern "C" bool i2c_reg8_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
  Wire.beginTransmission(dev_addr);
  Wire.write(reg_addr);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t requested = Wire.requestFrom((int)dev_addr, (int)len, (int)true);
  if (requested != len) {
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    if (!Wire.available()) return false;
    data[i] = (uint8_t)Wire.read();
  }
  return true;
}
