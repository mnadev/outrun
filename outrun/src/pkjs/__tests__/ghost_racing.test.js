/**
 * ghost_racing.test.js - Tests for Ghost Racing module
 */

describe('GhostRacing', () => {
  let GhostRacing;

  beforeEach(() => {
    // Clear all storage to ensure isolation
    localStorage.clear();
    jest.resetModules();
    GhostRacing = require('../ghost_racing');
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
      expect(result.id).toContain('ghost_');
    });
  });

  describe('loadGhosts', () => {
    it('returns empty array when no ghosts saved', () => {
      const ghosts = GhostRacing.loadGhosts();
      expect(Array.isArray(ghosts)).toBe(true);
    });
  });

  describe('deleteGhost', () => {
    it('deletes a ghost without error', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }

      const ghost = GhostRacing.saveGhost('Test', points, 300, 1000);

      // Should not throw
      expect(() => GhostRacing.deleteGhost(ghost.id)).not.toThrow();
    });
  });

  describe('startRacing', () => {
    it('returns false for non-existent ghost', () => {
      expect(GhostRacing.startRacing('fake-id')).toBe(false);
    });

    it('returns true with valid ghost', () => {
      const points = [];
      for (let i = 0; i < 10; i++) {
        points.push({ lat: 40.7 + i * 0.001, lng: -73.9, timestamp: i * 5000 });
      }
      const ghost = GhostRacing.saveGhost('Test', points, 300, 1000);

      expect(GhostRacing.startRacing(ghost.id)).toBe(true);
    });
  });

  describe('isRacing', () => {
    it('returns false initially', () => {
      expect(GhostRacing.isRacing()).toBe(false);
    });
  });

  describe('createMockGhosts', () => {
    it('creates demo ghosts array', () => {
      const ghosts = GhostRacing.createMockGhosts();
      expect(Array.isArray(ghosts)).toBe(true);
      expect(ghosts.length).toBeGreaterThan(0);
    });

    it('stores totalTime in milliseconds', () => {
      const ghosts = GhostRacing.createMockGhosts();
      // 5K in 25min -> 1500s -> 1,500,000ms. Regression: this used to be 1500
      // (seconds), which broke speed/finish math in getGhostPosition.
      expect(ghosts[0].totalTime).toBe(1500 * 1000);
    });
  });

  describe('getGhostPosition', () => {
    let Racing;
    const baseTime = 1000000;

    // Straight-line ghost so cumulative distance is predictable.
    function buildLinearGhost(numPoints, metersPerStep, msPerStep) {
      const points = [];
      const startLat = 40.7829;
      for (let i = 0; i < numPoints; i++) {
        // ~10m per 0.00009 deg latitude near NYC.
        const latStep = (metersPerStep / 10) * 0.00009;
        points.push({
          lat: startLat + i * latStep,
          lng: -73.9654,
          timestamp: i * msPerStep
        });
      }
      return {
        id: 'ghost_test',
        name: 'Test Ghost',
        date: baseTime,
        points: points,
        totalTime: msPerStep * (numPoints - 1),
        totalDistance: metersPerStep * (numPoints - 1)
      };
    }

    beforeEach(() => {
      localStorage.clear();
      jest.resetModules();
      Racing = require('../ghost_racing');
      jest.useFakeTimers();
      jest.setSystemTime(baseTime);
    });

    afterEach(() => {
      jest.useRealTimers();
    });

    it('returns null when not racing', () => {
      expect(Racing.getGhostPosition(40.7829, -73.9654, 0)).toBeNull();
    });

    it('reports ahead when the runner has covered more ground than the ghost', () => {
      const ghost = buildLinearGhost(11, 10, 1000); // 100m over 10s
      localStorage.setItem('outrun_ghost_runs', JSON.stringify([ghost]));
      Racing.startRacing(ghost.id);

      // 5s in: the ghost is ~50m along; the runner is at 70m -> ahead.
      jest.setSystemTime(baseTime + 5000);
      const pos = Racing.getGhostPosition(40.7829, -73.9654, 70);

      expect(pos).not.toBeNull();
      expect(pos.ghostFinished).toBe(false);
      expect(pos.distanceDiff).toBeGreaterThan(0);
      expect(pos.timeDiff).toBeGreaterThan(0);
    });

    it('reports behind when the runner has covered less ground than the ghost', () => {
      const ghost = buildLinearGhost(11, 10, 1000);
      localStorage.setItem('outrun_ghost_runs', JSON.stringify([ghost]));
      Racing.startRacing(ghost.id);

      jest.setSystemTime(baseTime + 5000);
      const pos = Racing.getGhostPosition(40.7829, -73.9654, 30);

      expect(pos).not.toBeNull();
      expect(pos.ghostFinished).toBe(false);
      expect(pos.distanceDiff).toBeLessThan(0);
      expect(pos.timeDiff).toBeLessThan(0);
    });

    it('reports finished once elapsed exceeds the ghost total time', () => {
      const ghost = buildLinearGhost(6, 10, 1000); // 50m over 5s = 5000ms
      localStorage.setItem('outrun_ghost_runs', JSON.stringify([ghost]));
      Racing.startRacing(ghost.id);

      // Past the ghost's total time -> the previously-unreachable finished
      // branch must now fire.
      jest.setSystemTime(baseTime + 6000);
      const pos = Racing.getGhostPosition(40.7829, -73.9654, 999);

      expect(pos).not.toBeNull();
      expect(pos.ghostFinished).toBe(true);
    });
  });
});
