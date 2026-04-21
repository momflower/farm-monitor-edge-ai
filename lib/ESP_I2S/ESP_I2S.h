#pragma once
#include <Arduino.h>
#include <driver/i2s.h>

enum i2s_mode_compat_t {
  I2S_MODE_STD = 0,
};

enum i2s_data_bit_width_compat_t {
  I2S_DATA_BIT_WIDTH_16BIT_COMPAT = 16,
};

enum i2s_slot_mode_compat_t {
  I2S_SLOT_MODE_MONO_COMPAT = 1,
  I2S_SLOT_MODE_STEREO_COMPAT = 2,
};

enum i2s_std_slot_mask_compat_t {
  I2S_STD_SLOT_LEFT_COMPAT = 1,
  I2S_STD_SLOT_RIGHT_COMPAT = 2,
};

class I2SClass {
public:
  I2SClass(i2s_port_t port = I2S_NUM_0);
  void setPins(int8_t bck, int8_t ws, int8_t dout, int8_t din = -1, int8_t mck = -1);
  bool begin(i2s_mode_compat_t mode, uint32_t sample_rate, i2s_data_bit_width_compat_t bits,
             i2s_slot_mode_compat_t slot_mode, i2s_std_slot_mask_compat_t slot_mask);
  size_t write(const uint8_t* data, size_t len);
  void end();
private:
  i2s_port_t _port;
  int8_t _bck=-1,_ws=-1,_dout=-1,_din=-1,_mck=-1;
  bool _begun=false;
};
