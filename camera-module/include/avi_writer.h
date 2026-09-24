#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

// Minimal Motion-JPEG AVI writer. Frames are written as they arrive, then
// finalize() backfills the RIFF/movi sizes and frame count. Chosen over
// H.264 because the ESP32-S3 has no hardware video encoder and the camera
// driver already emits JPEG per frame - concatenating them into MJPEG/AVI
// needs no re-encoding.
class AviWriter {
public:
  bool begin(const String &path, uint16_t width, uint16_t height, uint8_t fps);
  bool addFrame(const uint8_t *jpeg, size_t len);
  void finalize();

private:
  File file_;
  uint32_t frameCount_ = 0;
  uint32_t moviSizeOffset_ = 0;
  uint32_t moviDataStart_ = 0;
  uint16_t width_ = 0, height_ = 0;
  uint8_t fps_ = 5;
  std::vector<uint32_t> frameSizes_;
};
