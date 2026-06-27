var GpsKalman = require('../gps_kalman');

var M_PER_DEG = 6371000 * Math.PI / 180;

// Deterministic PRNG (mulberry32) so noise tests are reproducible.
function makeRng(seed) {
  return function () {
    seed |= 0;
    seed = (seed + 0x6D2B79F5) | 0;
    var t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

describe('GpsKalman', function () {
  test('first update returns the input position and zero speed', function () {
    var k = new GpsKalman();
    var e = k.update(40.0, -73.0, 5, 0);
    expect(e.lat).toBeCloseTo(40.0, 6);
    expect(e.lng).toBeCloseTo(-73.0, 6);
    expect(e.speed).toBe(0);
  });

  test('converges to the true speed on a constant-velocity track', function () {
    var k = new GpsKalman();
    var lat = 40.0;
    var dLat = 3 / M_PER_DEG; // 3 m/s due north, 1s steps
    k.update(lat, -73.0, 5, 0);
    var speed = 0;
    for (var i = 0; i < 60; i++) {
      lat += dLat;
      speed = k.update(lat, -73.0, 5, 1).speed;
    }
    expect(speed).toBeGreaterThan(2.7);
    expect(speed).toBeLessThan(3.3);
  });

  test('tracks the true position better than the raw noisy fixes', function () {
    var k = new GpsKalman();
    var rng = makeRng(42);
    var lat0 = 40.0;
    var dLat = 3 / M_PER_DEG;
    var noiseDeg = 8 / M_PER_DEG; // ~8m position noise
    k.update(lat0, -73.0, 8, 0);

    var sumRawErr = 0;
    var sumEstErr = 0;
    for (var i = 1; i <= 80; i++) {
      var trueLat = lat0 + i * dLat;
      var measLat = trueLat + (rng() - 0.5) * 2 * noiseDeg;
      var est = k.update(measLat, -73.0, 8, 1);
      sumRawErr += Math.abs((measLat - trueLat) * M_PER_DEG);
      sumEstErr += Math.abs((est.lat - trueLat) * M_PER_DEG);
    }
    // Filtered positions are, on average, closer to truth than the raw fixes.
    expect(sumEstErr).toBeLessThan(sumRawErr);
  });

  test('reset clears the filter so a new track re-initializes', function () {
    var k = new GpsKalman();
    k.update(40.0, -73.0, 5, 0);
    k.update(40.001, -73.0, 5, 1);
    k.reset();
    var e = k.update(10.0, 20.0, 5, 0);
    expect(e.lat).toBeCloseTo(10.0, 6);
    expect(e.lng).toBeCloseTo(20.0, 6);
    expect(e.speed).toBe(0);
  });
});
