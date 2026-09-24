# Ambulance Unit (ESP32)

Streams GPS location to the server over MQTT, sends periodic SMS location updates, and receives assignment messages from the server.

## Hardware

- ESP32 (generic DevKit assumed — adjust `platformio.ini` board id if different)
- GPS module — UART
- SIM800L GSM module — UART, own stable power supply

## Pin assignments (defaults in `include/config.h`) — **confirm against your wiring before flashing**

| Signal | GPIO |
|---|---|
| GPS RX/TX | 16 / 17 |
| GSM RX/TX | 18 / 19 |
| GSM reset | 5 |

An SH1106 OLED can optionally be added later, reusing `vehicle-unit`'s `oled.cpp` driver — not wired by default here since the brief marks it optional for this unit.

## Behavior

1. Publishes `{device_id, timestamp, lat, lon, speed_kmh, status}` to `ambulance/{DEVICE_ID}/location` every `LOCATION_PUBLISH_INTERVAL_MS` (default 7s).
2. Subscribes to `ambulance/{DEVICE_ID}/assignment`; when the server assigns this ambulance to an accident, logs the accident id/ETA and marks its own `status` as `assigned` on the next location publish.
3. Sends a plain-text SMS with an OSM link to `DISPATCH_PHONE_NUMBER` every `SMS_LOCATION_INTERVAL_MS` (default 5 min), independent of the MQTT channel.
4. MQTT goes over WiFi only, same as the vehicle unit (a phone hotspot is fine) — GSM is used solely for SMS, never as a data/MQTT transport (`mqtt_client.cpp`).

## Build

```bash
pio run
pio run -t upload
pio device monitor
```
