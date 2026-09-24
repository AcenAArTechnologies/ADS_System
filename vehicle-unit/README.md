# Vehicle Unit (ESP32-S3)

Reads MPU6050 + GPS, runs threshold-based crash detection, alerts via SMS + MQTT, and triggers the [camera-module](../camera-module) to save footage.

## Hardware

- ESP32-S3 (DevKitC-1 assumed — adjust `platformio.ini` board id for your exact module)
- NEO-6M (or similar) GPS module — UART
- SIM800L GSM module — UART, needs its own stable 3.7-4.2V/2A supply (not the ESP32 3.3V rail)
- MPU6050 accelerometer/gyroscope — I2C
- SH1106 128x64 OLED — I2C (shares the bus with the MPU6050)
- Buzzer, and a manual cancel/reset push button

## Pin assignments (defaults in `include/config.h`) — **confirm against your wiring before flashing**

| Signal | GPIO | Notes |
|---|---|---|
| GPS RX/TX | 16 / 17 | UART1 |
| GSM RX/TX | 18 / 19 | UART2 |
| GSM reset | 5 | optional; set to -1 if unused |
| MPU6050 SDA/SCL | 8 / 9 | I2C, shared with OLED |
| OLED SDA/SCL | 8 / 9 | same bus as MPU6050 |
| Buzzer | 4 | active HIGH |
| Cancel button | 6 | active LOW, internal pull-up |
| Camera trigger out | 7 | pulses HIGH ~200ms to signal the camera module |

## Behavior

1. Polls MPU6050 every 50ms; if acceleration deviates from 1g by more than `IMPACT_G_THRESHOLD`, or rotation exceeds `GYRO_DPS_THRESHOLD`, enters a 10s cancellable countdown (buzzer sounds).
2. If not cancelled via the button, fires the camera trigger pulse, sends an SMS with an OSM link, and publishes the accident event to MQTT.
3. MQTT publish goes over WiFi only (a phone hotspot is fine) — see `mqtt_client.cpp`. The SIM800L is used exclusively for SMS, never as a data/MQTT transport. Failed publishes are queued (up to `MQTT_RETRY_QUEUE_MAX`) and retried once WiFi/the broker is back.
4. The crash-detection function (`mpuDetectCrash` in `mpu6050.cpp`) is isolated so a TFLite Micro classifier can later replace the threshold logic without touching `main.cpp`.

## Build

```bash
pio run           # build
pio run -t upload # flash
pio device monitor
```

Edit `include/config.h` first: WiFi (hotspot) credentials, MQTT broker host/port, and the emergency SMS number.
