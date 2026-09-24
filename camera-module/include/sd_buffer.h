#pragma once
#include <Arduino.h>

bool sdBufferInit();

// Call every loop iteration when idle: captures a frame into the rolling
// pre-trigger buffer at CAPTURE_FPS. No-op while a clip is being recorded.
void sdBufferPollIdle();

// Call to begin saving a clip: freezes the current pre-trigger buffer and
// starts capturing POST_TRIGGER_FRAMES more frames.
void sdBufferStartRecording();

// Call every loop iteration while recording; returns true once the clip has
// been fully written to SD (path available via sdBufferLastClipPath()).
bool sdBufferPollRecording();

bool sdBufferIsRecording();
String sdBufferLastClipPath();
