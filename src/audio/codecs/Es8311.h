#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

// ES8311 (DAC/speaker path) bring-up over I2C, address 0x18.
//
// Register sequence and clock-divider coefficients are transcribed from
// Espressif's real, public `es8311` driver
// (https://github.com/espressif/esp-bsp/blob/master/components/es8311/es8311.c,
// Apache-2.0), not reconstructed from memory -- unlike an earlier version
// of this file. Only the single clock-divider table row needed for this
// board's fixed configuration (MCLK=12.288MHz, 24kHz, 16-bit, MCLK
// supplied externally on the MCLK pin) is transcribed; if the sample rate
// ever changes, pull the matching row from the real `coeff_div[]` table
// rather than guessing.
class Es8311 {
 public:
  // sampleRateHz must match what AudioCodec configured the I2S peripheral
  // for, and the I2S mclk_multiple must be 512 (12.288MHz MCLK) -- see
  // AudioCodec::i2sInit()'s comment for why.
  bool begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz);

  // 0-100.
  void setVolume(uint8_t volumePercent);
  void setMute(bool mute);

 private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &outValue);
  bool readModifyWrite(uint8_t reg, uint8_t clearMask, uint8_t setBits);

  i2c_master_dev_handle_t dev_ = nullptr;
};
