#include "Es7210.h"

#include <Arduino.h>

#include "../../core/Logger.h"

// Register addresses transcribed from
// https://github.com/espressif/esp-bsp/blob/master/components/es7210/priv_include/es7210_reg.h
namespace {
constexpr const char *kTag = "Es7210";
constexpr uint16_t kI2cAddress = 0x40;
constexpr uint32_t kI2cTimeoutMs = 100;

constexpr uint8_t kRegReset = 0x00;
constexpr uint8_t kRegMainClk = 0x02;
constexpr uint8_t kRegLrckDivH = 0x04;
constexpr uint8_t kRegLrckDivL = 0x05;
constexpr uint8_t kRegPowerDown = 0x06;
constexpr uint8_t kRegOsr = 0x07;
constexpr uint8_t kRegTimeControl0 = 0x09;
constexpr uint8_t kRegTimeControl1 = 0x0A;
constexpr uint8_t kRegSdpInterface1 = 0x11;
constexpr uint8_t kRegSdpInterface2 = 0x12;
constexpr uint8_t kRegAdc34Hpf2 = 0x20;
constexpr uint8_t kRegAdc34Hpf1 = 0x21;
constexpr uint8_t kRegAdc12Hpf2 = 0x22;
constexpr uint8_t kRegAdc12Hpf1 = 0x23;
constexpr uint8_t kRegAnalog = 0x40;
constexpr uint8_t kRegMic12Bias = 0x41;
constexpr uint8_t kRegMic34Bias = 0x42;
constexpr uint8_t kRegMic1Gain = 0x43;
constexpr uint8_t kRegMic2Gain = 0x44;
constexpr uint8_t kRegMic3Gain = 0x45;
constexpr uint8_t kRegMic4Gain = 0x46;
constexpr uint8_t kRegMic1Power = 0x47;
constexpr uint8_t kRegMic2Power = 0x48;
constexpr uint8_t kRegMic3Power = 0x49;
constexpr uint8_t kRegMic4Power = 0x4A;
constexpr uint8_t kRegMic12Power = 0x4B;
constexpr uint8_t kRegMic34Power = 0x4C;

// 30dB gain (ES7210_MIC_GAIN_30DB=10 in the real driver's enum) | 0x10,
// exactly as es7210_set_mic_gain() writes it.
constexpr uint8_t kMicGain30dbReg = 10 | 0x10;
// 2.87V bias (ES7210_MIC_BIAS_2V87 in the real driver's enum).
constexpr uint8_t kMicBias2v87 = 0x70;
}  // namespace

bool Es7210::begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz) {
  i2c_device_config_t devCfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kI2cAddress,
      .scl_speed_hz = 100000,
  };
  if (i2c_master_bus_add_device(bus, &devCfg, &dev_) != ESP_OK) {
    Logger::error(kTag, "i2c_master_bus_add_device failed");
    return false;
  }

  if (sampleRateHz != 24000) {
    Logger::warn(kTag,
                 "only the 24kHz/12.288MHz-MCLK clock table row is "
                 "transcribed -- other sample rates need a different row "
                 "from the real es7210_coeff_div[] table (see header)");
  }

  // Software reset, verbatim from es7210_config_codec().
  writeReg(kRegReset, 0xFF);
  writeReg(kRegReset, 0x32);

  writeReg(kRegTimeControl0, 0x30);
  writeReg(kRegTimeControl1, 0x30);

  writeReg(kRegAdc12Hpf1, 0x2A);
  writeReg(kRegAdc12Hpf2, 0x0A);
  writeReg(kRegAdc34Hpf1, 0x2A);
  writeReg(kRegAdc34Hpf2, 0x0A);

  // I2S format, 16-bit, TDM disabled (i2s_format=0x00 | 16-bit=0x60).
  writeReg(kRegSdpInterface1, 0x00 | 0x60);
  writeReg(kRegSdpInterface2, 0x00);

  writeReg(kRegAnalog, 0xC3);  // analog power + VMID voltage

  writeReg(kRegMic12Bias, kMicBias2v87);
  writeReg(kRegMic34Bias, kMicBias2v87);

  writeReg(kRegMic1Gain, kMicGain30dbReg);
  writeReg(kRegMic2Gain, kMicGain30dbReg);
  writeReg(kRegMic3Gain, kMicGain30dbReg);
  writeReg(kRegMic4Gain, kMicGain30dbReg);

  writeReg(kRegMic1Power, 0x08);
  writeReg(kRegMic2Power, 0x08);
  writeReg(kRegMic3Power, 0x08);
  writeReg(kRegMic4Power, 0x08);

  // Clock dividers for MCLK=12.288MHz, 24kHz (es7210_coeff_div[] row:
  // adc_div=1, dll=1, doubler=0, osr=0x20, lrck_h=0x02, lrck_l=0x00).
  writeReg(kRegOsr, 0x20);
  writeReg(kRegMainClk, 0x01 | (0 << 6) | (1 << 7));  // adc_div|doubler<<6|dll<<7
  writeReg(kRegLrckDivH, 0x02);
  writeReg(kRegLrckDivL, 0x00);

  writeReg(kRegPowerDown, 0x04);  // power down DLL (per real driver, post-DLL-lock)

  writeReg(kRegMic12Power, 0x0F);
  writeReg(kRegMic34Power, 0x0F);

  // Enable device, verbatim from es7210_config_codec().
  writeReg(kRegReset, 0x71);
  writeReg(kRegReset, 0x41);

  Logger::info(kTag, "begin() complete");
  return true;
}

bool Es7210::writeReg(uint8_t reg, uint8_t value) {
  if (!dev_) return false;
  uint8_t data[2] = {reg, value};
  return i2c_master_transmit(dev_, data, sizeof(data), kI2cTimeoutMs) ==
         ESP_OK;
}

bool Es7210::readReg(uint8_t reg, uint8_t &outValue) {
  if (!dev_) return false;
  uint8_t data = 0;
  if (i2c_master_transmit_receive(dev_, &reg, 1, &data, 1, kI2cTimeoutMs) !=
      ESP_OK) {
    return false;
  }
  outValue = data;
  return true;
}
