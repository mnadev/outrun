/**
 * gps_kalman.js - 2D constant-velocity Kalman filter for GPS smoothing.
 *
 * Smooths noisy GPS fixes and estimates ground speed. It uses a CONSTANT-VELOCITY
 * motion model so steady running is predicted (and therefore not discounted as
 * noise) -- a position-only smoother would systematically under-count distance
 * while moving, which is exactly wrong for a running tracker.
 *
 * The model has no cross-coupling between east and north, so it runs as two
 * independent 1-D position+velocity filters (kept tiny and explicit instead of
 * a 4x4 matrix library).
 *
 * Tuning -- accelNoise (m/s^2): how quickly speed/direction can change. Higher
 * follows the GPS more closely (less lag, noisier pace); lower is smoother (more
 * lag). The default 0.4 was chosen from synthetic-track measurements: a clean
 * track is tracked exactly (no distance bias), a 3->5 m/s change settles in ~9s,
 * reported pace noise is ~±20 s/km (so the watch's +/-10 s/km band with a 5s
 * debounce rarely false-triggers), and a heavily noisy track's accumulated
 * distance error drops from naive's ~+43% to ~-11%.
 */

var EARTH_RADIUS_M = 6371000;
var M_PER_DEG = EARTH_RADIUS_M * Math.PI / 180; // meters per degree of latitude
var INIT_VEL_VAR = 100; // (m/s)^2: velocity starts unknown, converges quickly

// One axis: state [p, v]; covariance [[Ppp, Ppv], [Pvp, Pvv]] (stays symmetric).
function Axis() {
  this.p = 0;
  this.v = 0;
  this.Ppp = 0;
  this.Ppv = 0;
  this.Pvp = 0;
  this.Pvv = 0;
}

Axis.prototype.init = function (p, posVar, velVar) {
  this.p = p;
  this.v = 0;
  this.Ppp = posVar;
  this.Ppv = 0;
  this.Pvp = 0;
  this.Pvv = velVar;
};

Axis.prototype.step = function (measurement, dt, measVar, accelVar) {
  // --- Predict (constant velocity): x' = F x, P' = F P F^T + Q ---
  var p = this.p + this.v * dt;
  var v = this.v;
  var Ppp = this.Ppp + dt * (this.Ppv + this.Pvp) + dt * dt * this.Pvv;
  var Ppv = this.Ppv + dt * this.Pvv;
  var Pvp = this.Pvp + dt * this.Pvv;
  var Pvv = this.Pvv;

  // Process noise from white-noise acceleration.
  var dt2 = dt * dt;
  var dt3 = dt2 * dt;
  var dt4 = dt3 * dt;
  Ppp += accelVar * dt4 / 4;
  Ppv += accelVar * dt3 / 2;
  Pvp += accelVar * dt3 / 2;
  Pvv += accelVar * dt2;

  // --- Update with a position measurement (H = [1, 0]) ---
  var S = Ppp + measVar;
  var Kp = Ppp / S;
  var Kv = Pvp / S;
  var residual = measurement - p;

  this.p = p + Kp * residual;
  this.v = v + Kv * residual;
  this.Ppp = (1 - Kp) * Ppp;
  this.Ppv = (1 - Kp) * Ppv;
  this.Pvp = Pvp - Kv * Ppp;
  this.Pvv = Pvv - Kv * Ppv;
};

function GpsKalman(options) {
  options = options || {};
  this.accelNoise = options.accelNoise || 0.4; // m/s^2 (see header for tuning)
  this.reset();
}

GpsKalman.prototype.reset = function () {
  this.initialized = false;
  this.lat0 = 0;
  this.lng0 = 0;
  this.mPerDegLng = M_PER_DEG;
  this.east = new Axis();
  this.north = new Axis();
};

/**
 * Fold one GPS fix into the filter.
 * @param {number} lat
 * @param {number} lng
 * @param {number} accuracy horizontal accuracy in meters (measurement std dev)
 * @param {number} dtSeconds time since the previous fix (ignored on first call)
 * @returns {{lat:number, lng:number, speed:number}} smoothed position + m/s speed
 */
GpsKalman.prototype.update = function (lat, lng, accuracy, dtSeconds) {
  var measVar = accuracy > 0 ? accuracy * accuracy : 1;

  if (!this.initialized) {
    // Anchor a local east/north meter frame at the first fix.
    this.lat0 = lat;
    this.lng0 = lng;
    this.mPerDegLng = M_PER_DEG * Math.cos(lat * Math.PI / 180);
    this.east.init(0, measVar, INIT_VEL_VAR);
    this.north.init(0, measVar, INIT_VEL_VAR);
    this.initialized = true;
    return { lat: lat, lng: lng, speed: 0 };
  }

  var dt = dtSeconds > 0 ? dtSeconds : 0.001;
  var e = (lng - this.lng0) * this.mPerDegLng;
  var n = (lat - this.lat0) * M_PER_DEG;
  var accelVar = this.accelNoise * this.accelNoise;

  this.east.step(e, dt, measVar, accelVar);
  this.north.step(n, dt, measVar, accelVar);

  var speed = Math.sqrt(this.east.v * this.east.v + this.north.v * this.north.v);
  return {
    lat: this.lat0 + this.north.p / M_PER_DEG,
    lng: this.lng0 + this.east.p / this.mPerDegLng,
    speed: speed
  };
};

module.exports = GpsKalman;
