#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

// Minimal ES8311 (DAC/speaker path) bring-up over I2C, address 0x18.
//
// CONFIDENCE NOTE: this register sequence is a best-effort reconstruction
// from the widely-reused public ES8311 init pattern (the same chip/sequence
// shape appears across many open-source ESP32 reference designs), NOT
// copied from a datasheet or the upstream `es8311` ESP-IDF component source
// in this session. It has NOT been verified against a datasheet here.
// begin() reads back the chip ID register as a sanity check that something
// ES8311-shaped is actually responding at 0x18 before proceeding, but a
// clean ID read does not guarantee every format/clock register below is
// bit-perfect. Treat audio quality/volume/pitch issues as "needs register
// tuning against the real datasheet," not necessarily "hardware is broken."
class Es8311 {
 public:
  // sampleRateHz must match what AudioCodec configured the I2S peripheral
  // for (they share the same bit/word clocks).
  bool begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz);

  // 0-100.
  void setVolume(uint8_t volumePercent);
  void setMute(bool mute);

 private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &outValue);

  i2c_master_dev_handle_t dev_ = nullptr;
};
