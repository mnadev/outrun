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

  console.log('Racing against ghost: ' + ghost.name);
  return true;
}

/**
 * Get ghost position at current time
 * Returns the ghost's position and time difference
 * @param {number} currentLat
 * @param {number} currentLng
 * @returns {Object|null} {ghostLat, ghostLng, timeDiff, distanceDiff}
 */
function getGhostPosition(currentLat, currentLng) {
  if (!currentGhost || !ghostStartTime) {
    return null;
  }

  // Calculate elapsed time since race start
  var elapsed = Date.now() - ghostStartTime;

  // Find ghost position at this elapsed time
  var points = currentGhost.points;
  while (ghostIndex < points.length - 1 && points[ghostIndex + 1].timestamp < elapsed) {
    ghostIndex++;
  }

  if (ghostIndex >= points.length) {
    // Ghost finished
    return {
      ghostLat: points[points.length - 1].lat,
      ghostLng: points[points.length - 1].lng,
      ghostFinished: true,
      timeDiff: 0
    };
  }

  var ghostPoint = points[ghostIndex];

  // Calculate approximate distance to ghost
  var distToGhost = haversineDistance(currentLat, currentLng, ghostPoint.lat, ghostPoint.lng);

  // Estimate time difference (roughly how far ahead/behind)
  // Positive = ahead of ghost, Negative = behind ghost
  var avgSpeed = currentGhost.totalDistance / currentGhost.totalTime;
  var timeDiff = distToGhost / avgSpeed;

  // Determine if ahead or behind based on ghost's progress
  var ghostProgress = ghostPoint.timestamp / currentGhost.totalTime;
  var runnerProgress = elapsed / currentGhost.totalTime;

  if (runnerProgress < ghostProgress) {
    timeDiff = -timeDiff; // Behind
  }

  return {
    ghostLat: ghostPoint.lat,
    ghostLng: ghostPoint.lng,
    distanceDiff: Math.round(distToGhost),
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
 * Create mock ghosts for demo/testing
 */
function createMockGhosts() {
  var mockGhosts = [
    {
      id: 'ghost_demo_fast',
      name: 'Your Best 5K',
      date: Date.now() - 86400000, // Yesterday
      points: generateMockPoints(5000, 1500), // 5K in 25min
      totalTime: 1500,
      totalDistance: 5000
    },
    {
      id: 'ghost_demo_rival',
      name: 'Sarah\'s Segment',
      date: Date.now() - 172800000, // 2 days ago
      points: generateMockPoints(2000, 480), // 2K in 8min
      totalTime: 480,
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
