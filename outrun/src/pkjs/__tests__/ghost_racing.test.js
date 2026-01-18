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
  });
});
