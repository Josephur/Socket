#include "Es8311.h"

#include <Arduino.h>

#include "../../core/Logger.h"

// Register addresses transcribed from
// https://github.com/espressif/esp-bsp/blob/master/components/es8311/priv_include/es8311_reg.h
namespace {
constexpr const char *kTag = "Es8311";
constexpr uint16_t kI2cAddress = 0x18;
constexpr uint32_t kI2cTimeoutMs = 100;

constexpr uint8_t kRegReset = 0x00;
constexpr uint8_t kRegClkManager1 = 0x01;
constexpr uint8_t kRegClkManager2 = 0x02;
constexpr uint8_t kRegClkManager3 = 0x03;
constexpr uint8_t kRegClkManager4 = 0x04;
constexpr uint8_t kRegClkManager5 = 0x05;
constexpr uint8_t kRegClkManager6 = 0x06;
constexpr uint8_t kRegClkManager7 = 0x07;
constexpr uint8_t kRegClkManager8 = 0x08;
constexpr uint8_t kRegSdpIn = 0x09;
constexpr uint8_t kRegSdpOut = 0x0A;
constexpr uint8_t kRegSystem0D = 0x0D;
constexpr uint8_t kRegSystem0E = 0x0E;
constexpr uint8_t kRegSystem12 = 0x12;
constexpr uint8_t kRegSystem13 = 0x13;
constexpr uint8_t kRegAdc1C = 0x1C;
constexpr uint8_t kRegDacMute = 0x31;
constexpr uint8_t kRegDacVolume = 0x32;
constexpr uint8_t kRegDac37 = 0x37;
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
                " (0x83 is the known-good ES8311 value)")
                   .c_str());

  if (sampleRateHz != 24000) {
    Logger::warn(kTag,
                 "only the 24kHz/12.288MHz-MCLK clock table row is "
                 "transcribed -- other sample rates need a different row "
                 "from the real coeff_div[] table (see header)");
  }

  // Reset, then power-on command (0x80) -- this exact 3-step sequence,
  // including the power-on write, is required; a version of this file
  // that skipped the final 0x80 write produced complete silence despite
  // every other register matching.
  writeReg(kRegReset, 0x1F);
  delay(20);
  writeReg(kRegReset, 0x00);
  writeReg(kRegReset, 0x80);

  // Clock select: MCLK supplied externally on the MCLK pin (not derived
  // from BCLK), neither MCLK nor SCLK inverted -> reg01 = 0x3F, no extra
  // bits set.
  writeReg(kRegClkManager1, 0x3F);

  // Clock dividers for MCLK=12.288MHz, 24kHz sample rate (coeff_div[] row:
  // pre_div=2, pre_multi=0, adc_div=1, dac_div=1, fs_mode=0, lrck_h=0x00,
  // lrck_l=0xFF, bclk_div=4, adc_osr=0x10, dac_osr=0x10).
  writeReg(kRegClkManager2, 0x20);  // (pre_div-1)<<5 | pre_multi<<3
  writeReg(kRegClkManager3, 0x10);  // fs_mode<<6 | adc_osr
  writeReg(kRegClkManager4, 0x10);  // dac_osr
  writeReg(kRegClkManager5, 0x00);  // (adc_div-1)<<4 | (dac_div-1)
  writeReg(kRegClkManager6, 0x03);  // sclk not inverted | (bclk_div-1)
  writeReg(kRegClkManager7, 0x00);  // lrck_h
  writeReg(kRegClkManager8, 0xFF);  // lrck_l

  // Format: slave mode (default), 16-bit resolution on both SDP in/out.
  readModifyWrite(kRegReset, 0x40, 0x00);  // clear bit6 -> slave serial port
  writeReg(kRegSdpIn, 3 << 2);
  writeReg(kRegSdpOut, 3 << 2);

  // Power-up sequence, verbatim from the real driver's es8311_init().
  writeReg(kRegSystem0D, 0x01);  // power up analog circuitry
  writeReg(kRegSystem0E, 0x02);  // enable analog PGA, ADC modulator
  writeReg(kRegSystem12, 0x00);  // power up DAC
  writeReg(kRegSystem13, 0x10);  // enable output to HP drive
  writeReg(kRegAdc1C, 0x6A);     // ADC equalizer bypass, DC offset cancel
  writeReg(kRegDac37, 0x08);     // bypass DAC equalizer

  setMute(false);
  // 75% ~= +16.5dB with setVolume()'s -30..+32dB mapping (see its comment
  // for why the scale is dB-windowed rather than the esp-bsp formula --
  // the old mapping made everything under ~50% inaudible, which is what
  // the 100% -> 70% -> 25% -> 50% tuning history was actually fighting).
  // 75% was picked by ear during bring-up. Runtime control lives in
  // AudioTestPanel's volume slider; keep kDefaultVolumePercent in sync.
  setVolume(75);

  Logger::info(kTag, "begin() complete");
  return true;
}

void Es8311::setVolume(uint8_t volumePercent) {
  if (volumePercent > 100) volumePercent = 100;
  // The DAC volume register spans -95.5dB..+32dB in 0.5dB steps (reg =
  // (dB + 95.5) * 2). The esp-bsp formula ((volume*256/100)-1) spreads the
  // percent scale across that whole span, which puts everything below
  // ~50% under -32dB -- inaudible to a human, though the self-test mic
  // still "hears" it. Field finding to match: "0 to 50% = absolutely no
  // sound".
  //
  // Instead, map onto the humanly useful window: 0% = hard mute, then
  // 1..100% linear-in-dB from -30dB (faint but audible on this speaker;
  // a first pass at -45dB left everything under ~20% still inaudible) up
  // to the register's full +32dB so 100% matches the loudest this chip
  // can go (the historical "100%"). Note the top of the range digitally
  // clips the test tone (which sits ~8.7dB under full-scale) -- that
  // distortion is part of how the old max sounded, and it's accepted
  // here in exchange for maximum loudness being reachable.
  uint8_t reg;
  if (volumePercent == 0) {
    reg = 0;  // -95.5dB, effectively mute
  } else {
    // dB = -30 + 62*(percent/100)  ->  reg = 191 + 2*dB = 131 + 124*percent/100
    reg = static_cast<uint8_t>(131 + (124 * volumePercent) / 100);
  }
  writeReg(kRegDacVolume, reg);
}

void Es8311::setMute(bool mute) {
  // Real es8311_voice_mute(): read-modify-write bits 5+6 only.
  readModifyWrite(kRegDacMute, 0x60, mute ? 0x60 : 0x00);
}

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

bool Es8311::readModifyWrite(uint8_t reg, uint8_t clearMask,
                              uint8_t setBits) {
  uint8_t value = 0;
  if (!readReg(reg, value)) return false;
  value = (value & ~clearMask) | setBits;
  return writeReg(reg, value);
}
