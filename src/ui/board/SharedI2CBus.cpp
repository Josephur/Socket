#include "SharedI2CBus.h"

DEV_I2C_Port &SharedI2CBus() {
  static DEV_I2C_Port port = DEV_I2C_Init();
  return port;
}
