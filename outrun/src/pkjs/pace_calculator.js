/**
 * pace_calculator.js - Pace calculation from GPS data
 * 
 * Calculates running pace using a rolling window of GPS positions.
 * Uses Haversine formula for distance calculation.
 */

/**
 * Calculate distance between two GPS coordinates (Haversine formula)
 * @param {number} lat1 
 * @param {number} lng1 
 * @param {number} lat2 
 * @param {number} lng2 
 * @returns {number} Distance in meters
 */
function haversineDistance(lat1, lng1, lat2, lng2) {
  var R = 6371000; // Earth radius in meters
  var dLat = toRad(lat2 - lat1);
  var dLng = toRad(lng2 - lng1);

  var a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) *
    Math.sin(dLng / 2) * Math.sin(dLng / 2);

  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

function toRad(deg) {
  return deg * (Math.PI / 180);
}

// GPS filtering thresholds.
var MAX_ACCURACY_M = 30;   // drop fixes worse than this
var MAX_SPEED_MPS = 12.5;  // ~45 km/h; faster between two fixes is a GPS glitch
var MIN_MOVE_M = 2;        // ignore sub-2m movement as stationary jitter

/**
 * PaceCalculator - Tracks GPS positions and calculates pace
 */
function PaceCalculator() {
  this.locations = [];
  this.targetPace = 300; // Default 5:00/km
  this.windowSize = 5;   // Rolling window of 5 GPS points
  this.totalDistance = 0;
  this.startTime = null;
}

/**
 * Reset the calculator for a new run
 */
PaceCalculator.prototype.reset = function () {
  this.locations = [];
  this.totalDistance = 0;
  this.startTime = null;
};

/**
 * Add a new GPS location
 * @param {Object} location - {lat, lng, accuracy, timestamp}
 */
PaceCalculator.prototype.addLocation = function (location) {
  // Skip inaccurate readings
  if (location.accuracy > MAX_ACCURACY_M) {
    console.log('Skipping inaccurate GPS reading: ' + location.accuracy + 'm');
    return;
  }

  // Set start time on first accepted location
  if (this.startTime === null) {
    this.startTime = location.timestamp;
  }

  // Calculate distance from previous point
  if (this.locations.length > 0) {
    var prev = this.locations[this.locations.length - 1];
    var dist = haversineDistance(prev.lat, prev.lng, location.lat, location.lng);
    var dt = (location.timestamp - prev.timestamp) / 1000; // seconds

    // Reject out-of-order / duplicate timestamps: we can't derive a speed, and
    // it would otherwise produce a divide-by-zero or a bogus pace spike.
    if (dt <= 0) {
      console.log('Skipping out-of-order GPS sample');
      return;
    }

    // Reject implausibly fast jumps (GPS glitch / teleport). Speed-based rather
    // than a flat distance cap, so genuine movement across a sparse GPS gap
    // (e.g. 200m over 60s) is kept instead of silently dropped.
    if (dist > MAX_SPEED_MPS * dt) {
      console.log('Skipping GPS jump: ' + dist.toFixed(0) + 'm in ' + dt.toFixed(0) + 's');
      return;
    }

    // Treat sub-threshold movement as stationary jitter so standing still
    // doesn't accrue phantom distance. The point is still recorded (window time
    // keeps advancing) so pace reflects that you've slowed/stopped.
    if (dist < MIN_MOVE_M) {
      dist = 0;
    }

    this.totalDistance += dist;
    location.distance = dist;
  } else {
    location.distance = 0;
  }

  this.locations.push(location);

  // Trim to window size
  if (this.locations.length > this.windowSize) {
    this.locations.shift();
  }
};

/**
 * Calculate current pace (seconds per kilometer)
 * Uses a rolling window for smoothing
 * @returns {number} Pace in seconds/km, or 0 if insufficient data
 */
PaceCalculator.prototype.getCurrentPace = function () {
  if (this.locations.length < 2) {
    return 0;
  }

  var first = this.locations[0];
  var last = this.locations[this.locations.length - 1];

  // Calculate distance in this window
  var windowDistance = 0;
  for (var i = 1; i < this.locations.length; i++) {
    windowDistance += this.locations[i].distance;
  }

  // Calculate time in this window
  var windowTime = (last.timestamp - first.timestamp) / 1000; // seconds

  if (windowDistance < 1 || windowTime < 1) {
    return 0;
  }

  // Pace = seconds per kilometer
  var pace = (windowTime / windowDistance) * 1000;

  // Clamp to reasonable values
  if (pace < 120) pace = 120;  // Faster than 2:00/km is probably GPS error
  if (pace > 1200) pace = 1200; // Slower than 20:00/km is walking/stopped

  return Math.round(pace);
};

/**
 * Get total distance in meters
 */
PaceCalculator.prototype.getTotalDistance = function () {
  return Math.round(this.totalDistance);
};

/**
 * Get elapsed time in seconds
 */
PaceCalculator.prototype.getElapsedTime = function () {
  if (this.startTime === null || this.locations.length === 0) {
    return 0;
  }
  var last = this.locations[this.locations.length - 1];
  return Math.round((last.timestamp - this.startTime) / 1000);
};

/**
 * Get target pace
 */
PaceCalculator.prototype.getTargetPace = function () {
  return this.targetPace;
};

/**
 * Set target pace
 */
PaceCalculator.prototype.setTargetPace = function (pace) {
  this.targetPace = pace;
};

/**
 * Get average pace for entire run
 */
PaceCalculator.prototype.getAveragePace = function () {
  var distance = this.getTotalDistance();
  var time = this.getElapsedTime();

  if (distance < 1 || time < 1) {
    return 0;
  }

  return Math.round((time / distance) * 1000);
};

/**
 * Get GPS track for server sync
 * Returns array of {lat, lng, timestamp} with timestamps relative to start.
 * Reads the lat/lng fields that addLocation() actually stores (the geolocation
 * API uses coords.latitude/longitude, but addLocation normalizes to lat/lng).
 */
PaceCalculator.prototype.getGpsTrack = function () {
  if (!this.startTime || this.locations.length === 0) {
    return [];
  }

  var startTime = this.startTime;
  return this.locations.map(function (loc) {
    return {
      lat: loc.lat,
      lng: loc.lng,
      timestamp: loc.timestamp - startTime
    };
  });
};

module.exports = PaceCalculator;

