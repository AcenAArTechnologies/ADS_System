require('dotenv').config();

module.exports = {
  port: parseInt(process.env.PORT || '3000', 10),
  mqtt: {
    url: process.env.MQTT_URL || 'mqtt://localhost:1883',
    username: process.env.MQTT_USERNAME || undefined,
    password: process.env.MQTT_PASSWORD || undefined,
  },
  dbPath: process.env.DB_PATH || './data/ads.db',
  osrmBaseUrl: process.env.OSRM_BASE_URL || 'https://router.project-osrm.org',
  telegram: {
    botToken: process.env.TELEGRAM_BOT_TOKEN || '',
    chatId: process.env.TELEGRAM_CHAT_ID || '',
  },
  assignmentSearchRadiusKm: parseFloat(process.env.ASSIGNMENT_SEARCH_RADIUS_KM || '25'),
};
