const express = require('express');

const VALID_DIRECTIONS = new Set(['forward', 'backward', 'left', 'right', 'stop']);

function vehiclesRouter(mqttClient) {
  const router = express.Router();

  router.post('/:deviceId/drive', (req, res) => {
    const { direction, speed } = req.body || {};
    if (!VALID_DIRECTIONS.has(direction)) {
      return res.status(400).json({ error: `direction must be one of ${[...VALID_DIRECTIONS].join(', ')}` });
    }
    let speedValue = speed === undefined ? undefined : Number(speed);
    if (speedValue !== undefined && (!Number.isFinite(speedValue) || speedValue < 0 || speedValue > 255)) {
      return res.status(400).json({ error: 'speed must be a number between 0 and 255' });
    }

    const payload = { direction };
    if (speedValue !== undefined) payload.speed = speedValue;

    mqttClient.publish(`vehicle/${req.params.deviceId}/drive`, JSON.stringify(payload), { qos: 1 });
    res.json({ ok: true });
  });

  return router;
}

module.exports = vehiclesRouter;
