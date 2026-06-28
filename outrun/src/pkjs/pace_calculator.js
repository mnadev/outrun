/**
 * pace_calculator.js - Pace calculation from GPS data
 *
 * GPS fixes are run through a constant-velocity Kalman filter (gps_kalman.js):
 * distance accumulates between SMOOTHED positions (less jitter inflation) and
 * current pace comes from the filter's velocity estimate (smoother and lower-lag
 * than dividing distance over a fixed window). Gross outliers, out-of-order
 * samples, and inaccurate fixes are still rejected before the filter sees them.
 */

var GpsKalman = require('./gps_kalman');

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
  this.windowSize = 5;   // Recent smoothed points kept for the track/elapsed
  this.totalDistance = 0;
  this.startTime = null;
  this.currentSpeed = 0; // m/s, from the Kalman velocity estimate
  this.kalman = new GpsKalman();
}

/**
 * Reset the calculator for a new run
 */
PaceCalculator.prototype.reset = function () {
  this.locations = [];
  this.totalDistance = 0;
  this.startTime = null;
  this.currentSpeed = 0;
  this.kalman.reset();
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

  var rawLat = location.lat;
  var rawLng = location.lng;

  if (this.locations.length > 0) {
    var prev = this.locations[this.locations.length - 1];
    var dt = (location.timestamp - prev.timestamp) / 1000; // seconds

    // Reject out-of-order / duplicate timestamps: we can't derive a speed, and
    // it would otherwise divide by zero or spike the pace -- and would corrupt
    // the filter's dt.
    if (dt <= 0) {
      console.log('Skipping out-of-order GPS sample');
      return;
    }

    // Reject implausibly fast jumps (GPS glitch / teleport) against the RAW
    // previous fix, before the filter sees them, so one teleport can't yank the
    // estimate. Speed-based (not a flat cap) keeps genuine movement across a
    // sparse GPS gap (e.g. 200m over 60s).
    var rawDist = haversineDistance(prev.rawLat, prev.rawLng, rawLat, rawLng);
    if (rawDist > MAX_SPEED_MPS * dt) {
      console.log('Skipping GPS jump: ' + rawDist.toFixed(0) + 'm in ' + dt.toFixed(0) + 's');
      return;
    }

    var est = this.kalman.update(rawLat, rawLng, location.accuracy, dt);
    this.currentSpeed = est.speed;

    // Distance between consecutive SMOOTHED positions; sub-threshold steps are
    // treated as residual jitter so standing still doesn't accrue distance.
    var step = haversineDistance(prev.lat, prev.lng, est.lat, est.lng);
    if (step < MIN_MOVE_M) {
      step = 0;
    }
    this.totalDistance += step;

    location.lat = est.lat; // store smoothed position for distance/track
    location.lng = est.lng;
    location.distance = step;
  } else {
    var first = this.kalman.update(rawLat, rawLng, location.accuracy, 0);
    location.lat = first.lat; // equals raw on the first fix
    location.lng = first.lng;
    location.distance = 0;
    this.currentSpeed = 0;
  }

  // Keep the raw fix for the next outlier check (location.lat is now smoothed).
  location.rawLat = rawLat;
  location.rawLng = rawLng;

  this.locations.push(location);

  // Trim to window size
  if (this.locations.length > this.windowSize) {
    this.locations.shift();
  }
};

/**
 * Calculate current pace (seconds per kilometer) from the Kalman velocity
 * estimate. Smoother and lower-lag than dividing distance over a fixed window.
 * @returns {number} Pace in seconds/km, or 0 if insufficient data / stopped
 */
PaceCalculator.prototype.getCurrentPace = function () {
  // Need at least one post-init fix for a velocity estimate.
  if (this.locations.length < 2 || this.currentSpeed <= 0) {
    return 0;
  }

  // Pace = seconds per kilometer = 1000 / (meters per second).
  var pace = 1000 / this.currentSpeed;

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
 * Current ground speed in m/s from the Kalman estimate (0 before the first
 * post-init fix). Used to decide movement for auto-pause.
 */
PaceCalculator.prototype.getCurrentSpeed = function () {
  return this.currentSpeed;
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

