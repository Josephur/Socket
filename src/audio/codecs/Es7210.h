#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

// Minimal ES7210 (dual-mic ADC path) bring-up over I2C, address 0x40.
//
// CONFIDENCE NOTE: same caveat as Es8311.h, but weaker here -- the ES7210
// register map is less commonly reproduced in public reference code than
// ES8311's, so this reconstruction carries more uncertainty. If the
// loopback self-test shows the DAC/speaker side working but no signal ever
// comes back from the mic, this file's register values are the first
// suspect, not necessarily the physical hardware.
class Es7210 {
 public:
  bool begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz);

 private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &outValue);

  i2c_master_dev_handle_t dev_ = nullptr;
};
