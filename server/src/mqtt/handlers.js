const { v4: uuidv4 } = require('uuid');
const { Accidents, Ambulances } = require('../db/models');
const { assignNearestAmbulance } = require('../services/assignment');
const { broadcast } = require('../ws/broadcast');
const { notifyTelegram } = require('../services/telegram');

function topicParts(topic) {
  // vehicle/{device_id}/accident -> ['vehicle', device_id, 'accident']
  return topic.split('/');
}

function registerHandlers(mqttClient) {
  mqttClient.on('message', async (topic, payloadBuf) => {
    let payload;
    try {
      payload = JSON.parse(payloadBuf.toString());
    } catch (err) {
      console.error(`[mqtt] bad JSON on ${topic}:`, err.message);
      return;
    }

    const [root, deviceId, kind] = topicParts(topic);

    try {
      if (root === 'vehicle' && kind === 'accident') {
        await handleAccident(deviceId, payload, mqttClient);
      } else if (root === 'ambulance' && kind === 'location') {
        handleAmbulanceLocation(deviceId, payload);
      } else if (root === 'vehicle' && kind === 'status') {
        broadcast('vehicle_status', { device_id: deviceId, ...payload });
      }
    } catch (err) {
      console.error(`[mqtt] handler error for ${topic}:`, err);
    }
  });
}

async function handleAccident(deviceId, payload, mqttClient) {
  const accident = {
    id: uuidv4(),
    device_id: payload.device_id || deviceId,
    timestamp: payload.timestamp || new Date().toISOString(),
    lat: payload.lat,
    lon: payload.lon,
    severity: payload.severity || 'unknown',
    impact_g: payload.impact_g ?? null,
    video_ref: payload.video_ref ?? null,
  };
  const saved = Accidents.insert(accident);
  broadcast('accident_created', saved);
  await notifyTelegram(
    `🚨 Accident reported\nDevice: ${saved.device_id}\nLocation: https://www.openstreetmap.org/?mlat=${saved.lat}&mlon=${saved.lon}\nSeverity: ${saved.severity}\nTime (server): ${saved.created_at} UTC`
  );

  const assignment = await assignNearestAmbulance(saved);
  if (assignment) {
    broadcast('assignment_created', assignment);
    mqttClient.publish(
      `ambulance/${assignment.ambulance_id}/assignment`,
      JSON.stringify({
        accident_id: assignment.accident_id,
        lat: accident.lat,
        lon: accident.lon,
        distance_km: assignment.distance_km,
        eta_minutes: assignment.eta_minutes,
      }),
      { qos: 1 }
    );
  } else {
    console.warn(`[assignment] no available ambulance found for accident ${accident.id}`);
  }
}

function handleAmbulanceLocation(deviceId, payload) {
  const record = {
    device_id: payload.device_id || deviceId,
    lat: payload.lat,
    lon: payload.lon,
    speed_kmh: payload.speed_kmh ?? null,
    status: payload.status || null,
    timestamp: payload.timestamp || new Date().toISOString(),
  };
  Ambulances.upsertLocation(record);
  broadcast('ambulance_location', record);
}

module.exports = { registerHandlers };
