#include "AudioCodec.h"

#include <Arduino.h>
#include <driver/i2s_std.h>

#include "../core/Logger.h"
#include "../ui/board/SharedI2CBus.h"

namespace {
constexpr const char *kTag = "AudioCodec";
constexpr int kPaEnablePin = 53;
constexpr gpio_num_t kMclkPin = GPIO_NUM_13;
constexpr gpio_num_t kBclkPin = GPIO_NUM_12;
constexpr gpio_num_t kWsPin = GPIO_NUM_10;
constexpr gpio_num_t kDoutPin = GPIO_NUM_9;
constexpr gpio_num_t kDinPin = GPIO_NUM_11;

i2s_chan_handle_t g_txHandle = nullptr;
i2s_chan_handle_t g_rxHandle = nullptr;
}  // namespace

bool AudioCodec::begin(uint32_t sampleRateHz) {
  Logger::info(kTag, "begin() start");
  pinMode(kPaEnablePin, OUTPUT);
  setAmplifierEnabled(false);

  Logger::info(kTag, "calling i2sInit()");
  Logger::flush();
  if (!i2sInit(sampleRateHz)) {
    Logger::error(kTag, "I2S channel init failed");
    return false;
  }
  Logger::info(kTag, "i2sInit() succeeded, calling codecInit()");

  if (!codecInit(sampleRateHz)) {
    Logger::error(kTag, "codec I2C init failed -- see Es8311/Es7210 headers");
    return false;
  }

  setAmplifierEnabled(true);
  ready_ = true;
  return true;
}

bool AudioCodec::i2sInit(uint32_t sampleRateHz) {
  i2s_chan_config_t chanCfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chanCfg.auto_clear = true;
  if (i2s_new_channel(&chanCfg, &g_txHandle, &g_rxHandle) != ESP_OK) {
    return false;
  }

  i2s_std_config_t stdCfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRateHz),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                       I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = kMclkPin,
              .bclk = kBclkPin,
              .ws = kWsPin,
              .dout = kDoutPin,
              .din = kDinPin,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };

  if (i2s_channel_init_std_mode(g_txHandle, &stdCfg) != ESP_OK) return false;
  if (i2s_channel_init_std_mode(g_rxHandle, &stdCfg) != ESP_OK) return false;
  if (i2s_channel_enable(g_txHandle) != ESP_OK) return false;
  if (i2s_channel_enable(g_rxHandle) != ESP_OK) return false;

  return true;
}

bool AudioCodec::codecInit(uint32_t sampleRateHz) {
  DEV_I2C_Port &bus = SharedI2CBus();

  Logger::info(kTag, "initializing ES8311 (speaker/DAC path)");
  if (!dac_.begin(bus.bus, sampleRateHz)) {
    return false;
  }

  Logger::info(kTag, "initializing ES7210 (mic/ADC path)");
  if (!adc_.begin(bus.bus, sampleRateHz)) {
    return false;
  }

  return true;
}

void AudioCodec::setVolume(uint8_t volumePercent) {
  dac_.setVolume(volumePercent);
}

size_t AudioCodec::write(const int16_t *samples, size_t count) {
  if (!g_txHandle) return 0;
  size_t bytesWritten = 0;
  i2s_channel_write(g_txHandle, samples, count * sizeof(int16_t),
                     &bytesWritten, portMAX_DELAY);
  return bytesWritten / sizeof(int16_t);
}

size_t AudioCodec::read(int16_t *samples, size_t maxCount) {
  if (!g_rxHandle) return 0;
  size_t bytesRead = 0;
  i2s_channel_read(g_rxHandle, samples, maxCount * sizeof(int16_t),
                    &bytesRead, portMAX_DELAY);
  return bytesRead / sizeof(int16_t);
}

void AudioCodec::setAmplifierEnabled(bool enabled) {
  digitalWrite(kPaEnablePin, enabled ? HIGH : LOW);
}
