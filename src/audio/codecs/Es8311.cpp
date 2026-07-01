#include "Es8311.h"

#include <Arduino.h>

#include "../../core/Logger.h"

namespace {
constexpr const char *kTag = "Es8311";
constexpr uint16_t kI2cAddress = 0x18;
constexpr uint32_t kI2cTimeoutMs = 100;

// Register map (best-effort recollection, see header CONFIDENCE NOTE).
constexpr uint8_t kRegReset = 0x00;
constexpr uint8_t kRegClkManager1 = 0x01;
constexpr uint8_t kRegClkManager2 = 0x02;
constexpr uint8_t kRegClkManager3 = 0x03;
constexpr uint8_t kRegClkManager5 = 0x05;
constexpr uint8_t kRegSdpIn = 0x09;   // DAC format (speaker path)
constexpr uint8_t kRegSdpOut = 0x0A;  // ADC format (unused on ES8311 here)
constexpr uint8_t kRegSystem9 = 0x0D;
constexpr uint8_t kRegSystem10 = 0x0E;
constexpr uint8_t kRegSystem13 = 0x12;
constexpr uint8_t kRegSystemPowerUp = 0x13;
constexpr uint8_t kRegAdcPga = 0x14;
constexpr uint8_t kRegDacMute = 0x31;
constexpr uint8_t kRegDacVolume = 0x32;
constexpr uint8_t kRegChipId1 = 0xFD;
}  // namespace

bool Es8311::begin(i2c_master_bus_handle_t bus, uint32_t sampleRateHz) {
  i2c_device_config_t devCfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kI2cAddress,
      .scl_speed_hz = 100000,
  };
  if (i2c_master_bus_add_device(bus, &devCfg, &dev_) != ESP_OK) {
    Logger::error(kTag, "i2c_master_bus_add_device failed");
    return false;
  }

  uint8_t id = 0;
  if (!readReg(kRegChipId1, id)) {
    Logger::error(kTag, "chip ID read failed (no ACK at 0x18?)");
    return false;
  }
  Logger::info(kTag,
               ("chip ID reg 0xFD = 0x" + String(id, HEX) +
                " (informational -- not gating on an exact expected value)")
                   .c_str());

  // Reset, then bring clocks/analog up. See CONFIDENCE NOTE in the header --
  // this is a best-effort standard sequence, not datasheet-verified here.
  writeReg(kRegReset, 0x1F);
  delay(20);
  writeReg(kRegReset, 0x00);

  writeReg(kRegClkManager1, 0x30);  // MCLK from pin, BCLK enabled
  writeReg(kRegClkManager2, 0x00);  // no pre-divide
  writeReg(kRegClkManager3, 0x10);  // ADC oversample rate
  writeReg(kRegClkManager5, 0x00);  // ADC/DAC clock divider = /1

  writeReg(kRegSdpIn, 0x0C);   // 16-bit, I2S/Philips format (DAC path)
  writeReg(kRegSdpOut, 0x0C);  // 16-bit, I2S/Philips format (unused ADC path)

  writeReg(kRegSystem9, 0x01);
  writeReg(kRegSystem10, 0x02);
  writeReg(kRegSystem13, 0x00);
  writeReg(kRegSystemPowerUp, 0x10);  // power up analog + DAC
  writeReg(kRegAdcPga, 0x00);         // ES7210 handles the real mic path

  setMute(false);
  setVolume(70);

  Logger::info(kTag, "begin() complete");
  return true;
}

void Es8311::setVolume(uint8_t volumePercent) {
  if (volumePercent > 100) volumePercent = 100;
  // 0x00-0xFF maps roughly linearly to -95.5dB..+32dB; 0xBF is commonly used
  // as the "0dB" reference point in reference designs.
  uint8_t reg = static_cast<uint8_t>((volumePercent * 0xBF) / 100);
  writeReg(kRegDacVolume, reg);
}

void Es8311::setMute(bool mute) { writeReg(kRegDacMute, mute ? 0x60 : 0x00); }

bool Es8311::writeReg(uint8_t reg, uint8_t value) {
  if (!dev_) return false;
  uint8_t data[2] = {reg, value};
  return i2c_master_transmit(dev_, data, sizeof(data), kI2cTimeoutMs) ==
         ESP_OK;
}

bool Es8311::readReg(uint8_t reg, uint8_t &outValue) {
  if (!dev_) return false;
  uint8_t data = 0;
  if (i2c_master_transmit_receive(dev_, &reg, 1, &data, 1, kI2cTimeoutMs) !=
      ESP_OK) {
    return false;
  }
  outValue = data;
  return true;
}
