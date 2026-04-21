#include "ESP_I2S.h"

I2SClass::I2SClass(i2s_port_t port):_port(port){}

void I2SClass::setPins(int8_t bck, int8_t ws, int8_t dout, int8_t din, int8_t mck){
  _bck=bck; _ws=ws; _dout=dout; _din=din; _mck=mck;
}

bool I2SClass::begin(i2s_mode_compat_t mode, uint32_t sample_rate, i2s_data_bit_width_compat_t bits,
                     i2s_slot_mode_compat_t slot_mode, i2s_std_slot_mask_compat_t slot_mask){
  (void)mode;
  if(_begun) end();
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sample_rate;
  cfg.bits_per_sample = (bits == I2S_DATA_BIT_WIDTH_16BIT_COMPAT) ? I2S_BITS_PER_SAMPLE_16BIT : I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = (slot_mode == I2S_SLOT_MODE_MONO_COMPAT) ?
    ((slot_mask == I2S_STD_SLOT_LEFT_COMPAT) ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_ONLY_RIGHT) :
    I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = sample_rate * 256;
  if (i2s_driver_install(_port, &cfg, 0, nullptr) != ESP_OK) return false;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = _mck;
  pins.bck_io_num = _bck;
  pins.ws_io_num = _ws;
  pins.data_out_num = _dout;
  pins.data_in_num = _din;
  if (i2s_set_pin(_port, &pins) != ESP_OK) {
    i2s_driver_uninstall(_port);
    return false;
  }
  i2s_zero_dma_buffer(_port);
  _begun = true;
  return true;
}

size_t I2SClass::write(const uint8_t* data, size_t len){
  if(!_begun) return 0;
  size_t written = 0;
  if (i2s_write(_port, data, len, &written, portMAX_DELAY) != ESP_OK) return 0;
  return written;
}

void I2SClass::end(){
  if(_begun){
    i2s_driver_uninstall(_port);
    _begun=false;
  }
}
