#pragma once

#include <cstddef>
#include <cstdint>

// ES8311 (DAC) + ES7210 (ADC, echo-cancelling dual mic) over I2S, control
// registers over the shared I2C bus. Pins per docs/PINOUT.md:
//   I2S: MCLK=13, BCLK=12, WS=10, DOUT=9, DIN=11
//   I2C (shared bus): SDA=7, SCL=8, ES8311=0x18, ES7210=0x40
//   PA enable: GPIO53 (active high)
//
// STATUS: the I2S peripheral setup below (i2sInit) uses the standard
// ESP-IDF i2s_std driver with this board's real pins and is expected to
// work as-is -- it mirrors docs/ESP-IDF/06_I2SCodec exactly.
//
// The ES8311/ES7210 codec *register* configuration is NOT implemented here.
// Espressif's reference example (docs/ESP-IDF/06_I2SCodec) configures the
// codec via the `es8311` ESP-IDF managed component
// (https://components.espressif.com/components/espressif/es8311), whose
// source isn't vendored into this repo yet and isn't something to
// hand-write from memory -- getting codec register sequences wrong can
// silently produce garbage audio. Before AudioCodec::begin() will produce
// real sound, vendor that component's es8311.c/.h into
// src/audio/es8311/ and wire codecInit() up to its es8311_create/
// es8311_init/es8311_sample_frequency_config/es8311_voice_volume_set calls,
// following the same shape as the IDF example.
class AudioCodec {
 public:
  bool begin(uint32_t sampleRateHz = 24000);

  size_t write(const int16_t *samples, size_t count);
  size_t read(int16_t *samples, size_t maxCount);

  void setAmplifierEnabled(bool enabled);

 private:
  bool i2sInit(uint32_t sampleRateHz);
  bool codecInit(uint32_t sampleRateHz);  // TODO: see class comment above.

  bool ready_ = false;
};
