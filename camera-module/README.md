# Camera Module (Seeed XIAO ESP32-S3 Sense)

Independent firmware. Keeps a rolling JPEG buffer, saves a 50s clip (20s pre-trigger + 30s post-trigger) to SD as a Motion-JPEG AVI on trigger, then uploads it to Telegram over its own WiFi connection. Communicates with the [vehicle-unit](../vehicle-unit) only through a single GPIO trigger signal — no shared network dependency.

## Hardware

- Seeed XIAO ESP32-S3 Sense (onboard OV2640 camera + onboard microSD slot, 8MB PSRAM)

## Wiring / trigger signal

Connect the Vehicle Unit's `CAMERA_TRIGGER_PIN` (GPIO7 on the vehicle unit, see its README) to this module's `TRIGGER_PIN` (GPIO1 by default, see `include/config.h`), with a shared ground between the two boards. The vehicle unit pulses this line HIGH for ~200ms on accident detection.

Camera and SD pins are fixed by the Sense expansion board and already set correctly in `config.h` — no wiring needed for those.

## Memory/storage constraints and the approach taken

The XIAO ESP32-S3 Sense has no hardware H.264 encoder, so "video" here means Motion-JPEG: each frame is captured as a JPEG by the camera driver and concatenated into an AVI container (`avi_writer.cpp`) with no re-encoding. Continuously buffering 50s of *raw* video would need far more RAM than is available; instead:

- A rolling ring buffer holds the last `PRE_TRIGGER_SECONDS` (20s) of JPEG frames in PSRAM (`sd_buffer.cpp`), captured at `CAPTURE_FPS` (default 5fps) and `FRAMESIZE_QVGA` (320x240).
- At 5fps/QVGA/quality 12, each JPEG frame is roughly 8-15KB, so the full 50s clip (250 frames) is ballpark 2-4MB — comfortably inside 8MB PSRAM alongside the camera driver's own frame buffers.
- On trigger, the pre-trigger buffer is flushed to the AVI file on SD, then `POST_TRIGGER_SECONDS` (30s) of new frames are captured and appended directly to the file (not held in RAM), so the only large in-memory allocation is the fixed-size pre-trigger ring.

If you need higher resolution/frame rate, raise `FRAMESIZE_QVGA`/`CAPTURE_FPS` in `config.h` incrementally and watch `ESP.getFreePsram()` in Serial output — PSRAM is the binding constraint, not SD speed or Telegram's upload limits (Bot API allows video files up to 50MB via `sendVideo`).

## Telegram bot setup

1. Message [@BotFather](https://t.me/BotFather) on Telegram, run `/newbot`, and follow the prompts to get a bot token.
2. Send any message to your new bot (or add it to a group/channel).
3. Visit `https://api.telegram.org/bot<token>/getUpdates` in a browser and find `"chat":{"id": ...}` in the response — that's your `TELEGRAM_CHAT_ID`.
4. Put both values into `include/config.h`.

## Behavior

1. Idle: captures a frame every `1/CAPTURE_FPS` seconds into the pre-trigger ring buffer.
2. On trigger (GPIO HIGH): flushes the ring buffer to a new `.avi` file under `/pending` on SD, then appends live frames until `POST_TRIGGER_FRAMES` is reached, then finalizes the AVI (backfilling RIFF/movi headers and the index).
3. Uploads the finished file to the configured Telegram chat via `sendVideo`, streamed directly from SD (never fully loaded into RAM).
4. If upload fails (no WiFi, request error), the file stays in `/pending` and is retried automatically the next time WiFi reconnects or the module reboots.

## Build

```bash
pio run
pio run -t upload
pio device monitor
```
