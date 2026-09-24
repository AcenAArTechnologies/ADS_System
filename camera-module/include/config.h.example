#pragma once

// ---- Identity ----
#define DEVICE_ID "CAM-001"

// ---- WiFi (this module's own connection, independent of the Vehicle Unit) ----
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// ---- Telegram ----
// Create a bot via @BotFather, get the token, then message the bot and read
// https://api.telegram.org/bot<token>/getUpdates to find your chat id.
#define TELEGRAM_BOT_TOKEN "123456789:your-bot-token"
#define TELEGRAM_CHAT_ID "your-chat-id"

// ---- Trigger input from the Vehicle Unit (active HIGH pulse) ----
#define TRIGGER_PIN 1  // one of the few free GPIOs on the XIAO ESP32-S3 Sense header

// ---- SD card (SDMMC 1-bit mode, onboard slot on the Sense expansion board) ----
#define SD_CLK_PIN 7
#define SD_CMD_PIN 9
#define SD_D0_PIN 8

// ---- Camera pins (Seeed XIAO ESP32-S3 Sense, OV2640) ----
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39
#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

// ---- Ring buffer / clip parameters ----
// QVGA @ ~5fps keeps a 50s clip (20s pre + 30s post) to roughly 2-4MB of
// JPEG frames, comfortably inside the module's 8MB PSRAM alongside the
// camera driver's own double buffer. Raise FRAMESIZE/FPS only after
// confirming free PSRAM headroom on your specific board revision.
#define CAPTURE_FPS 5
#define PRE_TRIGGER_SECONDS 20
#define POST_TRIGGER_SECONDS 30
#define PRE_TRIGGER_FRAMES (CAPTURE_FPS * PRE_TRIGGER_SECONDS)
#define POST_TRIGGER_FRAMES (CAPTURE_FPS * POST_TRIGGER_SECONDS)
#define JPEG_QUALITY 12 // 0-63, lower = higher quality/larger

#define PENDING_UPLOAD_DIR "/pending"
