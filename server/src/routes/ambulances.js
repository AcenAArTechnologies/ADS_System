const express = require('express');
const { Ambulances } = require('../db/models');

const router = express.Router();

router.get('/', (req, res) => {
  res.json(Ambulances.listAll());
});

router.get('/:deviceId', (req, res) => {
  const amb = Ambulances.get(req.params.deviceId);
  if (!amb) return res.status(404).json({ error: 'not found' });
  res.json(amb);
});

module.exports = router;
