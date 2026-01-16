/**
 * analytics.js - Fear Factor Analytics (Premium Feature)
 * 
 * Tracks and visualizes running performance with horror-themed metrics:
 * - Composure Score (how steady your pace is)
 * - Survival Rate (% of runs "escaped")
 * - Close Call Count (times in DANGER zone)
 * - Streak tracking
 */

var MockBackend = require('./mock_backend');

// Storage
var STORAGE_ANALYTICS = 'outrun_analytics';

/**
 * Analytics data structure
 */
var analyticsData = {
  sessions: [],         // Recent run sessions
  allTimeStats: {
    totalRuns: 0,
    totalDistance: 0,
    totalTime: 0,
    escapedCount: 0,
    caughtCount: 0,
    bestComposure: 0,
    worstComposure: 100,
    closeCallCount: 0,
    dangerTimeTotal: 0,
    longestStreak: 0,
    currentStreak: 0
  },
  weeklyStats: [],      // Last 7 days
  monthlyStats: []      // Last 30 days
};

/**
 * Initialize analytics
 */
function init() {
  var stored = localStorage.getItem(STORAGE_ANALYTICS);
  if (stored) {
    analyticsData = JSON.parse(stored);
  }
  console.log('Analytics initialized. Total runs: ' + analyticsData.allTimeStats.totalRuns);
}

/**
 * Save analytics to storage
 */
function save() {
  localStorage.setItem(STORAGE_ANALYTICS, JSON.stringify(analyticsData));
}

/**
 * Record a run session
 * @param {Object} session
 */
function recordSession(session) {
  var stats = analyticsData.allTimeStats;

  // Add to sessions (keep last 100)
  analyticsData.sessions.unshift({
    id: 'session_' + Date.now(),
    date: Date.now(),
    distance: session.distance || 0,
    time: session.time || 0,
    avgPace: session.avgPace || 0,
    composure: session.composure || 0,
    escaped: session.escaped || false,
    closeCalls: session.closeCalls || 0,
    dangerTime: session.dangerTime || 0
  });

  if (analyticsData.sessions.length > 100) {
    analyticsData.sessions = analyticsData.sessions.slice(0, 100);
  }

  // Update all-time stats
  stats.totalRuns++;
  stats.totalDistance += session.distance || 0;
  stats.totalTime += session.time || 0;

  if (session.escaped) {
    stats.escapedCount++;
    stats.currentStreak++;
    if (stats.currentStreak > stats.longestStreak) {
      stats.longestStreak = stats.currentStreak;
    }
  } else {
    stats.caughtCount++;
    stats.currentStreak = 0;
  }

  if (session.composure > stats.bestComposure) {
    stats.bestComposure = session.composure;
  }
  if (session.composure < stats.worstComposure) {
    stats.worstComposure = session.composure;
  }

  stats.closeCallCount += session.closeCalls || 0;
  stats.dangerTimeTotal += session.dangerTime || 0;

  save();
  console.log('Session recorded. Composure: ' + session.composure + ', Escaped: ' + session.escaped);
}

/**
 * Get survival rate (% of runs escaped)
 */
function getSurvivalRate() {
  var stats = analyticsData.allTimeStats;
  if (stats.totalRuns === 0) return 100;
  return Math.round((stats.escapedCount / stats.totalRuns) * 100);
}

/**
 * Get average composure score
 */
function getAverageComposure() {
  if (analyticsData.sessions.length === 0) return 0;

  var total = analyticsData.sessions.reduce(function (sum, s) {
    return sum + (s.composure || 0);
  }, 0);

  return Math.round(total / analyticsData.sessions.length);
}

/**
 * Get "Fear Factor" - composite score
 * Higher = more consistent under pressure
 */
function getFearFactor() {
  var survivalRate = getSurvivalRate();
  var avgComposure = getAverageComposure();
  var streakBonus = Math.min(analyticsData.allTimeStats.currentStreak * 2, 20);

  return Math.round((survivalRate * 0.4) + (avgComposure * 0.4) + streakBonus);
}

/**
 * Get recent sessions (last N)
 */
function getRecentSessions(count) {
  return analyticsData.sessions.slice(0, count || 10);
}

/**
 * Get all-time stats
 */
function getAllTimeStats() {
  return analyticsData.allTimeStats;
}

/**
 * Get weekly summary data
 */
function getWeeklySummary() {
  var weekAgo = Date.now() - (7 * 24 * 60 * 60 * 1000);
  var weekSessions = analyticsData.sessions.filter(function (s) {
    return s.date >= weekAgo;
  });

  if (weekSessions.length === 0) {
    return {
      runs: 0,
      distance: 0,
      avgComposure: 0,
      escaped: 0,
      caught: 0
    };
  }

  var totalComposure = weekSessions.reduce(function (sum, s) { return sum + s.composure; }, 0);
  var escaped = weekSessions.filter(function (s) { return s.escaped; }).length;

  return {
    runs: weekSessions.length,
    distance: weekSessions.reduce(function (sum, s) { return sum + s.distance; }, 0),
    avgComposure: Math.round(totalComposure / weekSessions.length),
    escaped: escaped,
    caught: weekSessions.length - escaped
  };
}

/**
 * Get horror-themed rank based on Fear Factor
 */
function getHorrorRank() {
  var fearFactor = getFearFactor();

  if (fearFactor >= 95) return { rank: 'Final Girl', emoji: '👑' };
  if (fearFactor >= 85) return { rank: 'Survivor', emoji: '🏆' };
  if (fearFactor >= 75) return { rank: 'Fighter', emoji: '💪' };
  if (fearFactor >= 60) return { rank: 'Runner', emoji: '🏃' };
  if (fearFactor >= 40) return { rank: 'Nervous', emoji: '😰' };
  if (fearFactor >= 20) return { rank: 'Stumbler', emoji: '😱' };
  return { rank: 'Victim', emoji: '💀' };
}

/**
 * Reset all analytics
 */
function reset() {
  analyticsData = {
    sessions: [],
    allTimeStats: {
      totalRuns: 0,
      totalDistance: 0,
      totalTime: 0,
      escapedCount: 0,
      caughtCount: 0,
      bestComposure: 0,
      worstComposure: 100,
      closeCallCount: 0,
      dangerTimeTotal: 0,
      longestStreak: 0,
      currentStreak: 0
    },
    weeklyStats: [],
    monthlyStats: []
  };
  save();
}

module.exports = {
  init: init,
  recordSession: recordSession,
  getSurvivalRate: getSurvivalRate,
  getAverageComposure: getAverageComposure,
  getFearFactor: getFearFactor,
  getRecentSessions: getRecentSessions,
  getAllTimeStats: getAllTimeStats,
  getWeeklySummary: getWeeklySummary,
  getHorrorRank: getHorrorRank,
  reset: reset
};
