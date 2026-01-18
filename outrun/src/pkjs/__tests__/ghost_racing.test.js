/**
 * ghost_racing.test.js - Tests for Ghost Racing module
 */

// Mock localStorage
const localStorageMock = {
  store: {},
  getItem: function (key) { return this.store[key] || null; },
  setItem: function (key, value) { this.store[key] = value; },
  removeItem: function (key) { delete this.store[key]; },
  clear: function () { this.store = {}; }
};
global.localStorage = localStorageMock;

const GhostRacing = require('../ghost_racing');

describe('GhostRacing', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  describe('saveGhost', () => {
    it('returns null with insufficient points', () => {
      const result = GhostRacing.saveGhost('Test', [
        { lat: 40.7, lng: -73.9, timestamp: 0 },
        { lat: 40.71, lng: -73.9, timestamp: 1000 }
      ], 60, 100);
      expect(result).toBeNull();
    });

    it('saves ghost with valid data', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const result = GhostRacing.saveGhost('My Run', points, 300, 1000);

      expect(result).not.toBeNull();
      expect(result.name).toBe('My Run');
      expect(result.totalTime).toBe(300);
      expect(result.totalDistance).toBe(1000);
    });

    it('normalizes timestamps relative to start', () => {
      const points = [];
      const startTime = Date.now();
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: startTime + i * 5000 });
      }
      const result = GhostRacing.saveGhost('Test', points, 300, 1000);

      expect(result.points[0].timestamp).toBe(0);
      expect(result.points[1].timestamp).toBe(5000);
    });

    it('limits stored ghosts to max count', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }

      // Save more than max (20)
      for (let i = 0; i < 25; i++) {
        GhostRacing.saveGhost('Run ' + i, points, 300, 1000);
      }

      const ghosts = GhostRacing.loadGhosts();
      expect(ghosts.length).toBeLessThanOrEqual(20);
    });
  });

  describe('loadGhosts', () => {
    it('returns empty array when no ghosts', () => {
      expect(GhostRacing.loadGhosts()).toEqual([]);
    });

    it('returns saved ghosts', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      GhostRacing.saveGhost('Test Run', points, 300, 1000);

      const ghosts = GhostRacing.loadGhosts();
      expect(ghosts.length).toBe(1);
      expect(ghosts[0].name).toBe('Test Run');
    });
  });

  describe('deleteGhost', () => {
    it('removes ghost by ID', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const ghost1 = GhostRacing.saveGhost('Run 1', points, 300, 1000);
      GhostRacing.saveGhost('Run 2', points, 300, 1000);

      GhostRacing.deleteGhost(ghost1.id);

      const ghosts = GhostRacing.loadGhosts();
      expect(ghosts.length).toBe(1);
      expect(ghosts[0].name).toBe('Run 2');
    });
  });

  describe('startRacing', () => {
    it('returns false for non-existent ghost', () => {
      expect(GhostRacing.startRacing('fake-id')).toBe(false);
    });

    it('returns true and sets current ghost', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const ghost = GhostRacing.saveGhost('Test', points, 300, 1000);

      expect(GhostRacing.startRacing(ghost.id)).toBe(true);
      expect(GhostRacing.isRacing()).toBe(true);
      expect(GhostRacing.getCurrentGhost().id).toBe(ghost.id);
    });
  });

  describe('stopRacing', () => {
    it('clears racing state', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const ghost = GhostRacing.saveGhost('Test', points, 300, 1000);
      GhostRacing.startRacing(ghost.id);

      GhostRacing.stopRacing();

      expect(GhostRacing.isRacing()).toBe(false);
      expect(GhostRacing.getCurrentGhost()).toBeNull();
    });
  });

  describe('getGhostPosition', () => {
    it('returns null when not racing', () => {
      expect(GhostRacing.getGhostPosition(40.7, -73.9)).toBeNull();
    });

    it('returns ghost position when racing', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const ghost = GhostRacing.saveGhost('Test', points, 45, 1000);
      GhostRacing.startRacing(ghost.id);

      const pos = GhostRacing.getGhostPosition(40.7, -73.9);

      expect(pos).not.toBeNull();
      expect(pos.ghostLat).toBeDefined();
      expect(pos.ghostLng).toBeDefined();
    });
  });

  describe('isRacing', () => {
    it('returns false initially', () => {
      expect(GhostRacing.isRacing()).toBe(false);
    });
  });

  describe('createMockGhosts', () => {
    it('creates demo ghosts', () => {
      const ghosts = GhostRacing.createMockGhosts();
      expect(ghosts.length).toBe(2);
      expect(ghosts[0].name).toBe("Your Best 5K");
      expect(ghosts[1].name).toBe("Sarah's Segment");
    });
  });
});
