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
        timestamp: base + 1000
      });
      calculator.addLocation({
        lat: 40.7138,
        lng: -74.0060,
        accuracy: 5,
        timestamp: base + 2000
      });

      const track = calculator.getGpsTrack();
      expect(track).toHaveLength(3);
      expect(track[0].lat).toBeCloseTo(40.7128);
      expect(track[0].lng).toBeCloseTo(-74.0060);
      expect(track[0].timestamp).toBe(0);
      expect(track[1].timestamp).toBe(1000);
      expect(track[2].lat).toBeCloseTo(40.7138);
      // Guard the original bug: coordinates must never be undefined.
      track.forEach((pt) => {
        expect(pt.lat).not.toBeUndefined();
        expect(pt.lng).not.toBeUndefined();
      });
    });
  });
});
