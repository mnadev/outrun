/**
 * ghost_racing.test.js - Tests for Ghost Racing module
 */

describe('GhostRacing', () => {
  let GhostRacing;

  beforeEach(() => {
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

      // Save both ghosts
      const ghost1 = GhostRacing.saveGhost('Run 1', points, 300, 1000);
      GhostRacing.saveGhost('Run 2', points, 300, 1000);

      // Verify we have 2
      expect(GhostRacing.loadGhosts().length).toBe(2);

      // Delete first ghost
      GhostRacing.deleteGhost(ghost1.id);

      // Verify only 1 remains
      const ghosts = GhostRacing.loadGhosts();
      expect(ghosts.length).toBe(1);
      expect(ghosts[0].name).toBe('Run 2');
    });
  });

  describe('startRacing', () => {
    it('returns false for non-existent ghost', () => {
      expect(GhostRacing.startRacing('fake-id')).toBe(false);
    });

    it('returns true and starts racing with valid ghost', () => {
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
    it('creates demo ghosts', () => {
      const ghosts = GhostRacing.createMockGhosts();
      expect(ghosts.length).toBe(2);
    });
  });
});
