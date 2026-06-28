/**
 * pace_calculator.test.js - Tests for pace calculator
 * 
 * Run with: npm test
 */

// Mock module.exports for Node.js testing
const PaceCalculator = require('../pace_calculator');

describe('PaceCalculator', () => {
  let calculator;

  beforeEach(() => {
    calculator = new PaceCalculator();
  });

  describe('initialization', () => {
    test('starts with default target pace of 5:00/km', () => {
      expect(calculator.getTargetPace()).toBe(300);
    });

    test('starts with zero distance', () => {
      expect(calculator.getTotalDistance()).toBe(0);
    });

    test('starts with zero elapsed time', () => {
      expect(calculator.getElapsedTime()).toBe(0);
    });
  });

  describe('addLocation', () => {
    test('ignores inaccurate readings over 30m', () => {
      calculator.addLocation({
        lat: 40.7128,
        lng: -74.0060,
        accuracy: 50,
        timestamp: Date.now()
      });
      expect(calculator.getTotalDistance()).toBe(0);
    });

    test('accepts accurate readings', () => {
      calculator.addLocation({
        lat: 40.7128,
        lng: -74.0060,
        accuracy: 10,
        timestamp: Date.now()
      });
      // First location, no distance yet
      expect(calculator.getTotalDistance()).toBe(0);
    });

    test('calculates distance between two points', () => {
      const now = Date.now();

      // Times Square, NYC
      calculator.addLocation({
        lat: 40.7580,
        lng: -73.9855,
        accuracy: 5,
        timestamp: now
      });

      // ~50m north (0.00045 lat = ~50m)
      calculator.addLocation({
        lat: 40.75845,
        lng: -73.9855,
        accuracy: 5,
        timestamp: now + 15000
      });

      const distance = calculator.getTotalDistance();
      expect(distance).toBeGreaterThan(30);
      expect(distance).toBeLessThan(70);
    });
  });

  describe('GPS outlier and jitter handling', () => {
    test('rejects an implausibly fast jump (GPS teleport)', () => {
      const now = Date.now();
      calculator.addLocation({ lat: 40.7580, lng: -73.9855, accuracy: 5, timestamp: now });
      // ~1.5km away only 2s later (~750 m/s) -- impossible, must be dropped.
      calculator.addLocation({ lat: 40.7715, lng: -73.9855, accuracy: 5, timestamp: now + 2000 });
      expect(calculator.getTotalDistance()).toBe(0);
    });

    test('keeps a plausible move across a sparse GPS gap', () => {
      const now = Date.now();
      calculator.addLocation({ lat: 40.7580, lng: -73.9855, accuracy: 5, timestamp: now });
      // ~200m over 60s (~3.3 m/s) is normal running; a flat 100m cap would have
      // wrongly discarded it.
      calculator.addLocation({ lat: 40.75980, lng: -73.9855, accuracy: 5, timestamp: now + 60000 });
      const d = calculator.getTotalDistance();
      expect(d).toBeGreaterThan(150);
      expect(d).toBeLessThan(250);
    });

    test('ignores sub-threshold jitter while stationary', () => {
      const now = Date.now();
      calculator.addLocation({ lat: 40.7580, lng: -73.9855, accuracy: 5, timestamp: now });
      // ~1m jitter back and forth around one spot -- should not accumulate.
      for (let i = 1; i <= 5; i++) {
        calculator.addLocation({
          lat: 40.7580 + (i % 2 === 0 ? 0.000005 : -0.000005), // ~0.5m offsets
          lng: -73.9855,
          accuracy: 5,
          timestamp: now + i * 5000
        });
      }
      expect(calculator.getTotalDistance()).toBe(0);
    });

    test('rejects out-of-order timestamps', () => {
      const now = Date.now();
      calculator.addLocation({ lat: 40.7580, lng: -73.9855, accuracy: 5, timestamp: now });
      calculator.addLocation({ lat: 40.75845, lng: -73.9855, accuracy: 5, timestamp: now - 5000 });
      expect(calculator.getTotalDistance()).toBe(0);
    });
  });

  describe('Kalman-smoothed distance and pace', () => {
    const M_PER_DEG = 6371000 * Math.PI / 180;

    // Deterministic PRNG so the noise test is reproducible.
    function makeRng(seed) {
      return function () {
        seed |= 0;
        seed = (seed + 0x6D2B79F5) | 0;
        let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
      };
    }

    test('accurate distance and pace on a clean constant-speed track', () => {
      const base = 1000000;
      const dLat = 3 / M_PER_DEG; // 3 m/s -> 5:33/km
      let lat = 40.0;
      calculator.addLocation({ lat, lng: -73.0, accuracy: 5, timestamp: base });
      for (let i = 1; i <= 60; i++) {
        lat += dLat;
        calculator.addLocation({ lat, lng: -73.0, accuracy: 5, timestamp: base + i * 1000 });
      }
      // True distance = 3 m/s * 60s = 180m; smoothing must not bias it much, and
      // crucially must NOT under-count (the constant-velocity model's whole point).
      const dist = calculator.getTotalDistance();
      expect(dist).toBeGreaterThan(170);
      expect(dist).toBeLessThan(190);
      // True pace = 1000/3 = 333 s/km.
      const pace = calculator.getCurrentPace();
      expect(pace).toBeGreaterThan(310);
      expect(pace).toBeLessThan(355);
    });

    test('reports ground speed for movement detection', () => {
      const base = 1000000;
      const dLat = 3 / M_PER_DEG; // 3 m/s
      let lat = 40.0;
      calculator.addLocation({ lat, lng: -73.0, accuracy: 5, timestamp: base });
      expect(calculator.getCurrentSpeed()).toBe(0); // no velocity from one fix
      for (let i = 1; i <= 30; i++) {
        lat += dLat;
        calculator.addLocation({ lat, lng: -73.0, accuracy: 5, timestamp: base + i * 1000 });
      }
      // Converges near the true 3 m/s -> clearly "moving".
      const speed = calculator.getCurrentSpeed();
      expect(speed).toBeGreaterThan(2.5);
      expect(speed).toBeLessThan(3.5);
    });

    test('noise does not massively inflate distance', () => {
      const base = 1000000;
      const rng = makeRng(7);
      const dLat = 3 / M_PER_DEG;
      const noiseDeg = 6 / M_PER_DEG; // ~6m position noise each fix
      const lat0 = 40.0;
      calculator.addLocation({ lat: lat0, lng: -73.0, accuracy: 8, timestamp: base });
      for (let i = 1; i <= 60; i++) {
        const measLat = lat0 + i * dLat + (rng() - 0.5) * 2 * noiseDeg;
        calculator.addLocation({ lat: measLat, lng: -73.0, accuracy: 8, timestamp: base + i * 1000 });
      }
      // True ~180m. Summing raw noisy fixes would inflate well past 200m; the
      // filter keeps the accumulated distance close to truth.
      const dist = calculator.getTotalDistance();
      expect(dist).toBeGreaterThan(150);
      expect(dist).toBeLessThan(210);
    });
  });

  describe('getCurrentPace', () => {
    test('returns 0 with insufficient data', () => {
      expect(calculator.getCurrentPace()).toBe(0);
    });

    test('calculates reasonable pace with valid data', () => {
      const now = Date.now();

      // Simulate running ~50m in 15 seconds (5:00/km pace)
      calculator.addLocation({
        lat: 40.7580,
        lng: -73.9855,
        accuracy: 5,
        timestamp: now
      });

      calculator.addLocation({
        lat: 40.75845,
        lng: -73.9855,
        accuracy: 5,
        timestamp: now + 15000
      });

      const pace = calculator.getCurrentPace();
      expect(pace).toBeGreaterThan(120);  // Faster than 2:00/km
      expect(pace).toBeLessThan(1200);    // Slower than 20:00/km
    });
  });

  describe('targetPace', () => {
    test('can set and get target pace', () => {
      calculator.setTargetPace(360);
      expect(calculator.getTargetPace()).toBe(360);
    });
  });

  describe('reset', () => {
    test('resets all state', () => {
      const now = Date.now();
      calculator.addLocation({
        lat: 40.7580,
        lng: -73.9855,
        accuracy: 5,
        timestamp: now
      });

      calculator.reset();

      expect(calculator.getTotalDistance()).toBe(0);
      expect(calculator.getElapsedTime()).toBe(0);
      expect(calculator.getCurrentPace()).toBe(0);
    });
  });

  describe('getGpsTrack', () => {
    test('returns empty array when no locations', () => {
      expect(calculator.getGpsTrack()).toEqual([]);
    });

    test('emits real lat/lng with timestamps relative to start', () => {
      const base = 1000000;
      // addLocation stores lat/lng (NOT coords.latitude/longitude), so the
      // track must read those same fields. Regression: it used to map
      // loc.latitude/loc.longitude and produced undefined coordinates.
      // Samples are ~56m / 20s apart (~2.8 m/s) so they pass the speed filter.
      calculator.addLocation({
        lat: 40.7128,
        lng: -74.0060,
        accuracy: 5,
        timestamp: base
      });
      calculator.addLocation({
        lat: 40.7133,
        lng: -74.0060,
        accuracy: 5,
        timestamp: base + 20000
      });
      calculator.addLocation({
        lat: 40.7138,
        lng: -74.0060,
        accuracy: 5,
        timestamp: base + 40000
      });

      const track = calculator.getGpsTrack();
      expect(track).toHaveLength(3);
      expect(track[0].lat).toBeCloseTo(40.7128);
      expect(track[0].lng).toBeCloseTo(-74.0060);
      expect(track[0].timestamp).toBe(0);
      expect(track[1].timestamp).toBe(20000);
      expect(track[2].lat).toBeCloseTo(40.7138);
      // Guard the original bug: coordinates must never be undefined.
      track.forEach((pt) => {
        expect(pt.lat).not.toBeUndefined();
        expect(pt.lng).not.toBeUndefined();
      });
    });
  });
});
