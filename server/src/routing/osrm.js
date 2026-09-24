const fetch = require('node-fetch');
const config = require('../config');

function haversineKm(lat1, lon1, lat2, lon2) {
  const R = 6371;
  const dLat = ((lat2 - lat1) * Math.PI) / 180;
  const dLon = ((lon2 - lon1) * Math.PI) / 180;
  const a =
    Math.sin(dLat / 2) ** 2 +
    Math.cos((lat1 * Math.PI) / 180) * Math.cos((lat2 * Math.PI) / 180) * Math.sin(dLon / 2) ** 2;
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}

/**
 * Driving distance/ETA between two points using OSRM's public/self-hosted route API.
 * Falls back to a haversine-based straight-line estimate (avg 40km/h) if OSRM is unreachable.
 */
async function routeDistanceEta(fromLat, fromLon, toLat, toLon) {
  const url = `${config.osrmBaseUrl}/route/v1/driving/${fromLon},${fromLat};${toLon},${toLat}?overview=full&geometries=geojson`;
  try {
    const res = await fetch(url, { timeout: 5000 });
    if (!res.ok) throw new Error(`OSRM HTTP ${res.status}`);
    const data = await res.json();
    if (data.code !== 'Ok' || !data.routes?.length) throw new Error('OSRM no route');
    const route = data.routes[0];
    return {
      distance_km: route.distance / 1000,
      eta_minutes: route.duration / 60,
      geometry: route.geometry, // GeoJSON LineString for drawing the route on the map
      source: 'osrm',
    };
  } catch (err) {
    const distanceKm = haversineKm(fromLat, fromLon, toLat, toLon);
    return {
      distance_km: distanceKm,
      eta_minutes: (distanceKm / 40) * 60, // assume 40 km/h average as a rough fallback
      geometry: null,
      source: 'haversine_fallback',
      error: err.message,
    };
  }
}

module.exports = { routeDistanceEta, haversineKm };
