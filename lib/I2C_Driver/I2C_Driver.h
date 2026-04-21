#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void I2C_Init(void);
bool i2c_reg8_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, size_t len);
bool i2c_reg8_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
