#include "sd_buffer.h"
#include "config.h"
#include "camera.h"
#include "avi_writer.h"
#include <SD_MMC.h>

struct StoredFrame {
  uint8_t *data = nullptr;
  size_t len = 0;
};

// Rolling pre-trigger buffer, allocated in PSRAM.
static StoredFrame preBuffer[PRE_TRIGGER_FRAMES];
static int preBufferHead = 0;   // next write index
static int preBufferCount = 0;

static bool recording = false;
static int postFramesCaptured = 0;
static uint32_t lastCaptureMs = 0;
static const uint32_t captureIntervalMs = 1000 / CAPTURE_FPS;
static AviWriter writer;
static String lastClipPath;
static int clipCounter = 0;

static void freeFrame(StoredFrame &f) {
  if (f.data) {
    free(f.data);
    f.data = nullptr;
    f.len = 0;
  }
}

static bool storeFrameInSlot(StoredFrame &slot, camera_fb_t *fb) {
  freeFrame(slot);
  slot.data = (uint8_t *)ps_malloc(fb->len);
  if (!slot.data) return false;
  memcpy(slot.data, fb->buf, fb->len);
  slot.len = fb->len;
  return true;
}

bool sdBufferInit() {
  if (!SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN)) return false;
  if (!SD_MMC.begin("/sdcard", true /* 1-bit mode */)) return false;
  if (!SD_MMC.exists(PENDING_UPLOAD_DIR)) SD_MMC.mkdir(PENDING_UPLOAD_DIR);
  return true;
}

void sdBufferPollIdle() {
  if (recording) return;
  uint32_t now = millis();
  if (now - lastCaptureMs < captureIntervalMs) return;
  lastCaptureMs = now;

  camera_fb_t *fb = cameraCaptureFrame();
  if (!fb) return;

  StoredFrame &slot = preBuffer[preBufferHead];
  if (storeFrameInSlot(slot, fb)) {
    preBufferHead = (preBufferHead + 1) % PRE_TRIGGER_FRAMES;
    if (preBufferCount < PRE_TRIGGER_FRAMES) preBufferCount++;
  }
  cameraReturnFrame(fb);
}

void sdBufferStartRecording() {
  if (recording) return;
  recording = true;
  postFramesCaptured = 0;

  clipCounter++;
  lastClipPath = String(PENDING_UPLOAD_DIR) + "/clip_" + String((uint32_t)time(nullptr)) + "_" + String(clipCounter) + ".avi";
  writer.begin(lastClipPath, 320, 240, CAPTURE_FPS);

  // Flush the pre-trigger buffer (oldest-first) into the AVI file.
  int idx = preBufferCount < PRE_TRIGGER_FRAMES ? 0 : preBufferHead;
  for (int i = 0; i < preBufferCount; i++) {
    StoredFrame &f = preBuffer[idx];
    if (f.data) writer.addFrame(f.data, f.len);
    idx = (idx + 1) % PRE_TRIGGER_FRAMES;
  }
}

bool sdBufferPollRecording() {
  if (!recording) return false;

  uint32_t now = millis();
  if (now - lastCaptureMs < captureIntervalMs) return false;
  lastCaptureMs = now;

  camera_fb_t *fb = cameraCaptureFrame();
  if (fb) {
    writer.addFrame(fb->buf, fb->len);
    cameraReturnFrame(fb);
    postFramesCaptured++;
  }

  if (postFramesCaptured >= POST_TRIGGER_FRAMES) {
    writer.finalize();
    recording = false;
    // Reset the pre-trigger ring so the next clip starts fresh.
    for (int i = 0; i < PRE_TRIGGER_FRAMES; i++) freeFrame(preBuffer[i]);
    preBufferHead = 0;
    preBufferCount = 0;
    return true;
  }
  return false;
}

bool sdBufferIsRecording() { return recording; }
String sdBufferLastClipPath() { return lastClipPath; }
