#pragma once
#include <Arduino.h>

// Uploads a file from SD to the configured Telegram chat via sendVideo.
// Streams directly from SD so the whole clip never needs to fit in RAM.
// Returns true and deletes the local file on success.
bool telegramUploadVideo(const String &sdPath);

// Scans PENDING_UPLOAD_DIR for clips left over from a failed upload (e.g. no
// WiFi at the time) and retries them. Call once after WiFi connects.
void telegramRetryPending();
