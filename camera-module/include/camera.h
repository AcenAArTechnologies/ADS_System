#pragma once
#include <Arduino.h>
#include <esp_camera.h>

bool cameraInit();
// Captures one JPEG frame. Caller must call cameraReturnFrame() when done.
camera_fb_t *cameraCaptureFrame();
void cameraReturnFrame(camera_fb_t *fb);
