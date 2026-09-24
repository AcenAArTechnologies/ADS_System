const { Accidents, Ambulances, Assignments } = require('../db/models');
const { routeDistanceEta, haversineKm } = require('../routing/osrm');
const config = require('../config');

/**
 * Nearest-ambulance assignment: filter to available ambulances within a straight-line
 * search radius, rank by straight-line distance, then confirm the top candidate with
 * an OSRM route distance (a full OSRM call per candidate would be slow/rate-limited
 * against the public demo server).
 */
async function assignNearestAmbulance(accident) {
  const candidates = Ambulances.listAvailable()
    .map((amb) => ({ amb, straightKm: haversineKm(accident.lat, accident.lon, amb.lat, amb.lon) }))
    .filter((c) => c.straightKm <= config.assignmentSearchRadiusKm)
    .sort((a, b) => a.straightKm - b.straightKm);

  if (!candidates.length) return null;

  const best = candidates[0];
  const route = await routeDistanceEta(best.amb.lat, best.amb.lon, accident.lat, accident.lon);

  Accidents.assign(accident.id, best.amb.device_id);
  Ambulances.setStatus(best.amb.device_id, 'assigned');
  Assignments.insert({
    accident_id: accident.id,
    ambulance_id: best.amb.device_id,
    distance_km: route.distance_km,
    eta_minutes: route.eta_minutes,
  });

  return {
    accident_id: accident.id,
    ambulance_id: best.amb.device_id,
    distance_km: route.distance_km,
    eta_minutes: route.eta_minutes,
    geometry: route.geometry,
  };
}

module.exports = { assignNearestAmbulance };
