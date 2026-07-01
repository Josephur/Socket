#include "DeviceIdentity.h"

namespace DeviceIdentity {

String chipId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[13];
  snprintf(buf, sizeof(buf), "%012llx", mac);
  return String(buf);
}

}  // namespace DeviceIdentity
