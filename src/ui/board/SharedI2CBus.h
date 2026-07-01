#pragma once

// All onboard peripherals (GT911 touch, ES8311/ES7210 codec, AXP2101 PMU,
// PCF85063 RTC, QMI8658 IMU) share one I2C bus on GPIO7 (SDA) / GPIO8 (SCL).
// This wraps the Waveshare DEV_I2C_Init() so it only runs once no matter how
// many modules need bus access.

#include "i2c.h"

// Returns the shared I2C master bus (lazily initialized on first call).
DEV_I2C_Port &SharedI2CBus();
