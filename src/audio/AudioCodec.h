#pragma once

#include <cstddef>
#include <cstdint>

#include "codecs/Es7210.h"
#include "codecs/Es8311.h"

// ES8311 (DAC) + ES7210 (ADC, echo-cancelling dual mic) over I2S, control
// registers over the shared I2C bus. Pins per docs/PINOUT.md:
//   I2S: MCLK=13, BCLK=12, WS=10, DOUT=9, DIN=11
//   I2C (shared bus): SDA=7, SCL=8, ES8311=0x18, ES7210=0x40
//   PA enable: GPIO53 (active high)
//
// STATUS: I2S peripheral setup (i2sInit) mirrors docs/ESP-IDF/06_I2SCodec
// with this board's real pins. Codec register init (codecInit) now does
// real I2C register writes via Es8311/Es7210 -- see those headers'
// CONFIDENCE NOTE. This has NOT been verified by an actual listening test
// yet; it's a best-effort reconstruction, not vendored/verified upstream
// source. Treat "wrong volume/pitch/noise" as a register-tuning problem,
// and "dead silence with no I2C errors" as also possibly a register
// problem before assuming the physical hardware is at fault.
class AudioCodec {
 public:
  bool begin(uint32_t sampleRateHz = 24000);

  size_t write(const int16_t *samples, size_t count);
  size_t read(int16_t *samples, size_t maxCount);

  void setAmplifierEnabled(bool enabled);
  void setVolume(uint8_t volumePercent);

 private:
  bool i2sInit(uint32_t sampleRateHz);
  bool codecInit(uint32_t sampleRateHz);

  Es8311 dac_;
  Es7210 adc_;
  bool ready_ = false;
};
