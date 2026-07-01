#pragma once

#include "RuntimeConfig.h"

namespace ProvisioningClient {

// HTTPS GETs `<provisioningUrl>?device_id=<chip id>` and parses the JSON
// response into `outConfig`. Expected response shape:
//   {
//     "device_name": "...",
//     "persona": "...",
//     "wake_word": "...",
//     "tts_voice": "...",
//     "converse_endpoint": "https://.../api/converse"
//   }
// Returns true and sets outConfig.loaded=true on success.
bool fetch(const String &provisioningUrl, RuntimeConfig &outConfig);

}  // namespace ProvisioningClient
