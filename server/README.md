# ADS Server

Backend for the Accident Detection, Alert & Ambulance Navigation System. Node.js (>=22.5, uses the built-in `node:sqlite` module — no native build step) + Express + MQTT.js, with OSRM-based routing and a Leaflet dashboard served statically.

## Setup

```bash
cp .env.example .env   # edit MQTT broker URL, OSRM URL, Telegram token if used
npm install
npm start               # or: npm run dev (auto-restart)
```

Requires an MQTT broker reachable at `MQTT_URL`. For local development:

```bash
docker run -it -p 1883:1883 eclipse-mosquitto
```

## Endpoints

- `GET /api/accidents?active=true` — list accidents
- `GET /api/accidents/:id` — accident detail + latest assignment
- `POST /api/accidents/:id/resolve` — mark resolved
- `POST /api/accidents/:id/video-ref` — attach Telegram video link `{ "video_ref": "https://t.me/..." }`
- `GET /api/ambulances` — list ambulances with last known location/status
- `GET /api/ambulances/:deviceId`
- `GET /ws` — WebSocket feed (`accident_created`, `ambulance_location`, `assignment_created`)
- `/` — Leaflet dashboard

See [docs/architecture.md](docs/architecture.md) for the full data flow, MQTT schema, and assignment algorithm.
