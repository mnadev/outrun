/**
 * ghost_racing.js - Ghost Racing Module (Premium Feature)
 * 
 * Allows racing against:
 * - Your own past runs
 * - Friends' segment times
 * - Virtual pacers
 */

var StravaAuth = require('./strava_auth');
var MockBackend = require('./mock_backend');

// Storage
var STORAGE_GHOST_RUNS = 'outrun_ghost_runs';
var MAX_STORED_RUNS = 20;

// Ghost state
var currentGhost = null;
var ghostIndex = 0;
var ghostStartTime = null;
// Cumulative distance (m) of the ghost up to each point index, built once per
// race so ahead/behind is a cheap distance-progress comparison, not a guess
// from wall-clock fractions. ghostCumulative[i] = distance covered by point i.
var ghostCumulative = null;

/**
 * Ghost run data structure
 * @typedef {Object} GhostRun
 * @property {string} id - Run ID
 * @property {string} name - Display name
 * @property {number} date - Timestamp
 * @property {Array<{lat, lng, timestamp}>} points - GPS points with relative timestamps
 * @property {number} totalTime - Total time in seconds
 * @property {number} totalDistance - Total distance in meters
 */

/**
 * Record current run as a ghost for future races
 * @param {string} name - Name for this ghost
 * @param {Array} points - GPS points from pace calculator
 * @param {number} totalTime
 * @param {number} totalDistance
 */
function saveGhost(name, points, totalTime, totalDistance) {
  if (points.length < 5) {
    console.log('Not enough points to save as ghost');
    return null;
  }

  // Normalize timestamps relative to start
  var startTime = points[0].timestamp;
  var normalizedPoints = points.map(function (p) {
    return {
      lat: p.lat,
      lng: p.lng,
      timestamp: p.timestamp - startTime
    };
  });

  var ghost = {
    id: 'ghost_' + Date.now(),
    name: name || 'Run ' + new Date().toLocaleDateString(),
    date: Date.now(),
    points: normalizedPoints,
    totalTime: totalTime,
    totalDistance: totalDistance
  };

  // Load existing ghosts
  var ghosts = loadGhosts();
  ghosts.unshift(ghost);

  // Trim to max
  if (ghosts.length > MAX_STORED_RUNS) {
    ghosts = ghosts.slice(0, MAX_STORED_RUNS);
  }

  localStorage.setItem(STORAGE_GHOST_RUNS, JSON.stringify(ghosts));
  console.log('Ghost saved: ' + ghost.name);

  return ghost;
}

/**
 * Load all saved ghosts
 * @returns {GhostRun[]}
 */
function loadGhosts() {
  var stored = localStorage.getItem(STORAGE_GHOST_RUNS);
  if (stored) {
    return JSON.parse(stored);
  }
  return [];
}

/**
 * Delete a ghost by ID
 */
function deleteGhost(ghostId) {
  var ghosts = loadGhosts();
  ghosts = ghosts.filter(function (g) { return g.id !== ghostId; });
  localStorage.setItem(STORAGE_GHOST_RUNS, JSON.stringify(ghosts));
}

/**
 * Start racing against a ghost
 * @param {string} ghostId
 * @returns {boolean}
 */
function startRacing(ghostId) {
  var ghosts = loadGhosts();
  var ghost = ghosts.find(function (g) { return g.id === ghostId; });

  if (!ghost) {
    console.log('Ghost not found: ' + ghostId);
    return false;
  }

  currentGhost = ghost;
  ghostIndex = 0;
  ghostStartTime = Date.now();
  ghostCumulative = buildCumulativeDistances(ghost.points);

  console.log('Racing against ghost: ' + ghost.name);
  return true;
}

/**
 * Cumulative distance covered along the ghost's track, indexed by point.
 * @param {Array} points ghost points with {lat, lng}
 * @returns {Array<number>} distances in meters, [0, d01, d012, ...]
 */
function buildCumulativeDistances(points) {
  if (!points || points.length === 0) {
    return [0];
  }
  var cumulative = [0];
  for (var i = 1; i < points.length; i++) {
    var step = haversineDistance(points[i - 1].lat, points[i - 1].lng,
                                 points[i].lat, points[i].lng);
    cumulative[i] = cumulative[i - 1] + step;
  }
  return cumulative;
}

/**
 * Get the ghost's position and the runner's lead/lag versus it.
 *
 * Ahead/behind is decided by DISTANCE PROGRESS at the same elapsed time, not by
 * comparing wall-clock fractions (which the old code did and which made the
 * runner read "ahead" almost always). All times are milliseconds.
 *
 * @param {number} currentLat
 * @param {number} currentLng
 * @param {number} runnerDistance total meters the runner has covered so far
 * @returns {Object|null} {ghostLat, ghostLng, timeDiff, distanceDiff, ghostFinished}
 */
function getGhostPosition(currentLat, currentLng, runnerDistance) {
  if (!currentGhost || !ghostStartTime) {
    return null;
  }

  var points = currentGhost.points;
  // elapsed and point timestamps are both in ms.
  var elapsed = Date.now() - ghostStartTime;
  var totalTimeMs = currentGhost.totalTime;

  // Advance to the last point at or before the current elapsed time.
  while (ghostIndex < points.length - 1 && points[ghostIndex + 1].timestamp <= elapsed) {
    ghostIndex++;
  }

  // Ghost finished once the runner has outlasted it. The old code's
  // ghostIndex >= points.length branch was unreachable (the loop caps at
  // length - 1), so the "done" state never fired.
  var lastTs = points[points.length - 1].timestamp;
  if (totalTimeMs > 0 && elapsed >= totalTimeMs) {
    return {
      ghostLat: points[points.length - 1].lat,
      ghostLng: points[points.length - 1].lng,
      ghostFinished: true,
      distanceDiff: 0,
      timeDiff: 0
    };
  }

  var ghostPoint = points[ghostIndex];
  var cumulative = ghostCumulative || buildCumulativeDistances(points);

  // Distance the ghost had covered by this elapsed time.
  var ghostDistance = cumulative[ghostIndex] || 0;

  // Positive distanceDiff = runner is ahead (covered more ground);
  // negative = behind. This is the real signal.
  var distanceDiff = (runnerDistance || 0) - ghostDistance;

  // Convert the distance gap to an approximate time gap using the ghost's
  // average speed in m/ms. Positive = ahead, negative = behind.
  var avgSpeed = totalTimeMs > 0 ? currentGhost.totalDistance / totalTimeMs : 0;
  var timeDiff = avgSpeed > 0 ? distanceDiff / avgSpeed : distanceDiff;

  return {
    ghostLat: ghostPoint.lat,
    ghostLng: ghostPoint.lng,
    distanceDiff: Math.round(distanceDiff),
    timeDiff: Math.round(timeDiff),
    ghostFinished: false
  };
}

/**
 * Stop racing ghost
 */
function stopRacing() {
  currentGhost = null;
  ghostIndex = 0;
  ghostStartTime = null;
  ghostCumulative = null;
}

/**
 * Check if currently racing a ghost
 */
function isRacing() {
  return currentGhost !== null;
}

/**
 * Get current ghost info
 */
function getCurrentGhost() {
  return currentGhost;
}

/**
 * Haversine distance helper
 */
function haversineDistance(lat1, lng1, lat2, lng2) {
  var R = 6371000;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLng = (lng2 - lng1) * Math.PI / 180;

  var a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
    Math.sin(dLng / 2) * Math.sin(dLng / 2);

  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

/**
 * Create mock ghosts for demo/testing.
 *
 * totalTime and point timestamps are both in MILLISECONDS, matching the units
 * real ghosts are saved with (index.js passes elapsed * 1000). The old values
 * were in seconds, which broke getGhostPosition's speed/finish math.
 */
function createMockGhosts() {
  var mockGhosts = [
    {
      id: 'ghost_demo_fast',
      name: 'Your Best 5K',
      date: Date.now() - 86400000, // Yesterday
      points: generateMockPoints(5000, 1500), // 5K in 25min (1500s)
      totalTime: 1500 * 1000,
      totalDistance: 5000
    },
    {
      id: 'ghost_demo_rival',
      name: 'Sarah\'s Segment',
      date: Date.now() - 172800000, // 2 days ago
      points: generateMockPoints(2000, 480), // 2K in 8min (480s)
      totalTime: 480 * 1000,
      totalDistance: 2000
    }
  ];

  localStorage.setItem(STORAGE_GHOST_RUNS, JSON.stringify(mockGhosts));
  return mockGhosts;
}

/**
 * Generate mock GPS points for demo
 */
function generateMockPoints(distance, time) {
  var points = [];
  var numPoints = Math.floor(time / 5); // Point every 5 seconds

  // Simple straight line (NYC Central Park direction)
  var startLat = 40.7829;
  var startLng = -73.9654;
  var latStep = 0.00009 * (distance / time); // ~10m per lat degree at NYC

  for (var i = 0; i < numPoints; i++) {
    points.push({
      lat: startLat + (i * latStep),
      lng: startLng,
      timestamp: i * 5000 // 5 second intervals
    });
  }

  return points;
}

module.exports = {
  saveGhost: saveGhost,
  loadGhosts: loadGhosts,
  deleteGhost: deleteGhost,
  startRacing: startRacing,
  getGhostPosition: getGhostPosition,
  stopRacing: stopRacing,
  isRacing: isRacing,
  getCurrentGhost: getCurrentGhost,
  createMockGhosts: createMockGhosts
};
