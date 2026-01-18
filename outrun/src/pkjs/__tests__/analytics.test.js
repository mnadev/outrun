/**
 * analytics.test.js - Tests for Fear Factor Analytics module
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

const Analytics = require('../analytics');

describe('Analytics', () => {
  beforeEach(() => {
    localStorage.clear();
    Analytics.reset();
    Analytics.init();
  });

  describe('initialization', () => {
    it('initializes with zero total runs', () => {
      const stats = Analytics.getAllTimeStats();
      expect(stats.totalRuns).toBe(0);
    });

    it('starts with 100% survival rate', () => {
      expect(Analytics.getSurvivalRate()).toBe(100);
    });

    it('starts with 0 average composure', () => {
      expect(Analytics.getAverageComposure()).toBe(0);
    });
  });

  describe('recordSession', () => {
    it('increments total run count', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 80, escaped: true });
      expect(Analytics.getAllTimeStats().totalRuns).toBe(1);
    });

    it('tracks escaped runs', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 80, escaped: true });
      expect(Analytics.getAllTimeStats().escapedCount).toBe(1);
    });

    it('tracks caught runs', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 40, escaped: false });
      expect(Analytics.getAllTimeStats().caughtCount).toBe(1);
    });

    it('updates best composure', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 95, escaped: true });
      expect(Analytics.getAllTimeStats().bestComposure).toBe(95);
    });

    it('updates worst composure', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 25, escaped: false });
      expect(Analytics.getAllTimeStats().worstComposure).toBe(25);
    });

    it('accumulates total distance', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 80, escaped: true });
      Analytics.recordSession({ distance: 3000, time: 1200, composure: 70, escaped: true });
      expect(Analytics.getAllTimeStats().totalDistance).toBe(8000);
    });

    it('accumulates total time', () => {
      Analytics.recordSession({ distance: 5000, time: 1800, composure: 80, escaped: true });
      Analytics.recordSession({ distance: 3000, time: 1200, composure: 70, escaped: true });
      expect(Analytics.getAllTimeStats().totalTime).toBe(3000);
    });
  });

  describe('streaks', () => {
    it('increments streak on escaped runs', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 85, escaped: true });
      expect(Analytics.getAllTimeStats().currentStreak).toBe(2);
    });

    it('resets streak on caught run', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 30, escaped: false });
      expect(Analytics.getAllTimeStats().currentStreak).toBe(0);
    });

    it('tracks longest streak', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 30, escaped: false });
      Analytics.recordSession({ composure: 80, escaped: true });
      expect(Analytics.getAllTimeStats().longestStreak).toBe(3);
    });
  });

  describe('getSurvivalRate', () => {
    it('returns 100% with no runs', () => {
      expect(Analytics.getSurvivalRate()).toBe(100);
    });

    it('calculates correct percentage', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 30, escaped: false });
      Analytics.recordSession({ composure: 80, escaped: true });
      expect(Analytics.getSurvivalRate()).toBe(75); // 3/4
    });
  });

  describe('getAverageComposure', () => {
    it('returns 0 with no sessions', () => {
      expect(Analytics.getAverageComposure()).toBe(0);
    });

    it('calculates correct average', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 60, escaped: true });
      Analytics.recordSession({ composure: 100, escaped: true });
      expect(Analytics.getAverageComposure()).toBe(80); // (80+60+100)/3
    });
  });

  describe('getHorrorRank', () => {
    it('returns Victim for low score', () => {
      Analytics.recordSession({ composure: 10, escaped: false });
      const rank = Analytics.getHorrorRank();
      expect(rank.rank).toBe('Victim');
      expect(rank.emoji).toBe('💀');
    });

    it('returns Final Girl for high score', () => {
      // Need high survival rate + high composure + streak
      for (let i = 0; i < 10; i++) {
        Analytics.recordSession({ composure: 98, escaped: true });
      }
      const rank = Analytics.getHorrorRank();
      expect(rank.rank).toBe('Final Girl');
      expect(rank.emoji).toBe('👑');
    });
  });

  describe('getRecentSessions', () => {
    it('returns empty array with no sessions', () => {
      expect(Analytics.getRecentSessions()).toEqual([]);
    });

    it('returns sessions in reverse chronological order', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 90, escaped: true });
      const sessions = Analytics.getRecentSessions(2);
      expect(sessions[0].composure).toBe(90);
      expect(sessions[1].composure).toBe(80);
    });

    it('limits to requested count', () => {
      for (let i = 0; i < 10; i++) {
        Analytics.recordSession({ composure: 50 + i, escaped: true });
      }
      expect(Analytics.getRecentSessions(5).length).toBe(5);
    });
  });

  describe('getWeeklySummary', () => {
    it('returns zero stats with no recent runs', () => {
      const summary = Analytics.getWeeklySummary();
      expect(summary.runs).toBe(0);
      expect(summary.distance).toBe(0);
    });

    it('includes runs from last 7 days', () => {
      Analytics.recordSession({ distance: 5000, composure: 80, escaped: true });
      const summary = Analytics.getWeeklySummary();
      expect(summary.runs).toBe(1);
      expect(summary.distance).toBe(5000);
    });
  });

  describe('reset', () => {
    it('clears all data', () => {
      Analytics.recordSession({ composure: 80, escaped: true });
      Analytics.recordSession({ composure: 90, escaped: true });
      Analytics.reset();
      expect(Analytics.getAllTimeStats().totalRuns).toBe(0);
      expect(Analytics.getRecentSessions()).toEqual([]);
    });
  });
});
