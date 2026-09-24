const express = require('express');
const { Accidents, Assignments } = require('../db/models');

const router = express.Router();

router.get('/', (req, res) => {
  const { active } = req.query;
  res.json(active === 'true' ? Accidents.listActive() : Accidents.listAll());
});

router.get('/:id', (req, res) => {
  const accident = Accidents.get(req.params.id);
  if (!accident) return res.status(404).json({ error: 'not found' });
  const assignment = Assignments.latestForAccident(accident.id);
  res.json({ ...accident, assignment });
});

router.post('/:id/resolve', (req, res) => {
  Accidents.resolve(req.params.id);
  res.json({ ok: true });
});

// Attach a Telegram message link / video reference forwarded from the camera module.
router.post('/:id/video-ref', (req, res) => {
  const { video_ref } = req.body || {};
  if (!video_ref) return res.status(400).json({ error: 'video_ref required' });
  Accidents.setVideoRef(req.params.id, video_ref);
  res.json({ ok: true });
});

module.exports = router;
