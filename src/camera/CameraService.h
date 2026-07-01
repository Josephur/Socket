#pragma once

// Interface-only stub. Per the plan's "Camera scope" decision: reserve a
// hook, write zero driver code until the MIPI-CSI camera module is actually
// in hand. The 15-pin connector and an OV5647-class sensor are expected, but
// today's Arduino core has no meaningful MIPI-CSI/ISP support (would need
// porting ESP-IDF's esp_video component) -- see the plan's "Key Risks".
//
// Deliberately no .cpp file: there is nothing to implement yet. When camera
// work starts, this interface is the contract the rest of the app (UI,
// conversation flow) should be written against, so plugging in a real
// implementation later doesn't ripple through other modules.
class CameraService {
 public:
  virtual ~CameraService() = default;

  virtual bool begin() = 0;
  virtual bool isAvailable() const = 0;

  // TODO: define capture/frame-access API once a concrete use case (visual
  // diagnostics, QR/ticket lookup, etc) is chosen.
};
