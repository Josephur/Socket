#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

// ES7210 (dual-mic ADC path) bring-up over I2C, address 0x40.
//
// Register sequence and clock-divider coefficients are transcribed from
// Espressif's real, public `es7210` driver
// (https://github.com/espressif/esp-bsp/blob/master/components/es7210/es7210.c,
// Apache-2.0), not reconstructed from memory. Only the single
// clock-divider table row needed for this board's fixed configuration
// (MCLK=12.288MHz, 24kHz, 16-bit I2S) is transcribed; if the sample rate
// ever changes, pull the matching row from the real `es7210_coeff_div[]`
// table rather than guessing.
class Es7210 {
 public:
  // sampleRateHz must match AudioCodec's I2S config, and the I2S
  // mclk_multiple must be 512 (12.288MHz MCLK) -- see
  // AudioCodec::i2sInit()'s comment for why (this codec's coefficient
  // table has no 24kHz entry at the 256x/6.144MHz default at all, which
  // was the original bug: silent/all-zero mic capture).
  bool begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz);

 private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &outValue);

  i2c_master_dev_handle_t dev_ = nullptr;
};
