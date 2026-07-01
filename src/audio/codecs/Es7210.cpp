#include "Es7210.h"

#include <Arduino.h>

#include "../../core/Logger.h"

namespace {
constexpr const char *kTag = "Es7210";
constexpr uint16_t kI2cAddress = 0x40;
constexpr uint32_t kI2cTimeoutMs = 100;

// Register map (best-effort recollection, see header CONFIDENCE NOTE --
// lower confidence than Es8311's).
constexpr uint8_t kRegReset = 0x00;
constexpr uint8_t kRegClockOff = 0x01;
constexpr uint8_t kRegMainClk = 0x02;
constexpr uint8_t kRegSdpInterface = 0x11;
constexpr uint8_t kRegAdcPower = 0x16;
constexpr uint8_t kRegMic1Gain = 0x43;
constexpr uint8_t kRegMic2Gain = 0x44;
constexpr uint8_t kRegChipId = 0xFD;
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

  uint8_t id = 0;
  if (!readReg(kRegChipId, id)) {
    Logger::error(kTag, "chip ID read failed (no ACK at 0x40?)");
    return false;
  }
  Logger::info(kTag, ("chip ID reg 0xFD = 0x" + String(id, HEX) +
                       " (informational only)")
                          .c_str());

  writeReg(kRegReset, 0xFF);
  delay(10);
  writeReg(kRegReset, 0x41);

  writeReg(kRegClockOff, 0x3F);  // enable internal clocks
  writeReg(kRegMainClk, 0x00);   // no clock pre-divide

  writeReg(kRegSdpInterface, 0x00);  // 16-bit I2S/Philips format
  writeReg(kRegAdcPower, 0x3F);      // power up all 4 ADC channels

  writeReg(kRegMic1Gain, 0x1A);  // ~30dB mic gain, both onboard mics
  writeReg(kRegMic2Gain, 0x1A);

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
