# Accident Detection, Alert & Ambulance Navigation System

IoT system: vehicle-mounted crash detection + camera evidence capture, ambulance GPS tracking, and a server that computes ambulance-to-accident routing/ETA using OpenStreetMap data only (no Google Maps, no paid APIs).

## Structure

| Folder | What it is |
|---|---|
| [`server/`](server) | Node.js backend — MQTT ingestion, SQLite storage, OSRM routing, REST/WebSocket API |
| [`dashboard/`](dashboard) | Leaflet + OSM live tracking page, served by the server |
| [`vehicle-unit/`](vehicle-unit) | ESP32-S3 firmware — crash detection, GPS, SMS/MQTT alerting |
| [`camera-module/`](camera-module) | XIAO ESP32-S3 Sense firmware — buffered clip capture, Telegram upload |
| [`ambulance-unit/`](ambulance-unit) | ESP32 firmware — GPS streaming, SMS location, assignment receiver |

Each firmware project and the server build/deploy independently. See [`server/docs/architecture.md`](server/docs/architecture.md) for the full data flow diagram, MQTT topic/payload schema, and the ambulance-assignment algorithm.

## Getting started

1. **Server**: `cd server && cp .env.example .env && npm install && npm start` (needs an MQTT broker — see server README).
2. **Dashboard**: opens automatically at `http://localhost:3000` once the server is running.
3. **Firmware**: for each of `vehicle-unit`, `ambulance-unit`, `camera-module`, edit `include/config.h` (WiFi/MQTT/GSM/Telegram credentials, and confirm GPIO pin assignments against your actual wiring), then `pio run -t upload`.

## Design constraints honored throughout

- Maps/routing: Leaflet + OpenStreetMap tiles for display, OSRM for driving distance/ETA — no Google Maps anywhere.
- Every unit (vehicle, camera, ambulance) has its own independent WiFi/hotspot connection and its own `WIFI_SSID`/`WIFI_PASSWORD` in its own `config.h` — they can be on entirely different networks. The only cross-unit link is the single GPIO trigger signal between the vehicle unit and its camera module. Distance/ETA between an ambulance and an accident is computed purely from each device's own GPS coordinates, never from which network it's on — see [Independent connectivity and distance/ETA](server/docs/architecture.md#independent-connectivity-and-distanceeta) for what that means for broker placement when units are on separate hotspots.
- Crash detection is threshold-based (v1) but isolated in one function (`mpuDetectCrash` in `vehicle-unit/src/mpu6050.cpp`) so a TFLite Micro model can replace it later without touching the rest of the firmware.
