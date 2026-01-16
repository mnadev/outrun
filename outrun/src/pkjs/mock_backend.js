/**
 * mock_backend.js - Mock backend for development
 * 
 * Simulates Firebase/backend functionality for:
 * - User profiles
 * - Leagues and teams
 * - Leaderboards with "Composure" scoring
 * - Activity sync
 * 
 * Replace with real Firebase integration when ready.
 */

// Storage keys
var STORAGE_USER_PROFILE = 'outrun_user_profile';
var STORAGE_LEAGUES = 'outrun_leagues';
var STORAGE_ACTIVITIES = 'outrun_activities';

// Mock data
var MOCK_LEAGUES = [
  {
    id: 'league_global',
    name: 'Global Survivors',
    isPrivate: false,
    teams: ['team_final_girls', 'team_axemen', 'team_scream_queens'],
    memberCount: 1247
  },
  {
    id: 'league_local',
    name: 'NYC Run Club',
    isPrivate: true,
    teams: ['team_central_park', 'team_brooklyn'],
    memberCount: 42
  }
];

var MOCK_TEAMS = {
  'team_final_girls': {
    id: 'team_final_girls',
    name: 'The Final Girls',
    weeklyMileage: 342.5,
    members: ['user1', 'user2', 'user3'],
    rank: 1
  },
  'team_axemen': {
    id: 'team_axemen',
    name: 'The Axemen',
    weeklyMileage: 298.2,
    members: ['user4', 'user5'],
    rank: 2
  },
  'team_scream_queens': {
    id: 'team_scream_queens',
    name: 'Scream Queens',
    weeklyMileage: 276.8,
    members: ['user6'],
    rank: 3
  }
};

var MOCK_LEADERBOARD = [
  { rank: 1, name: 'Sarah C.', composureScore: 98.5, escapedCount: 47, caughtCount: 2 },
  { rank: 2, name: 'Mike T.', composureScore: 95.2, escapedCount: 38, caughtCount: 3 },
  { rank: 3, name: 'Jamie L.', composureScore: 92.8, escapedCount: 29, caughtCount: 4 },
  { rank: 4, name: 'Alex R.', composureScore: 88.1, escapedCount: 22, caughtCount: 5 },
  { rank: 5, name: 'You', composureScore: 85.0, escapedCount: 15, caughtCount: 6 }
];

// User profile
var currentUser = null;

/**
 * Initialize mock backend
 */
function init() {
  // Load user profile from storage
  var stored = localStorage.getItem(STORAGE_USER_PROFILE);
  if (stored) {
    currentUser = JSON.parse(stored);
  } else {
    currentUser = createDefaultUser();
  }
  console.log('Mock backend initialized for user: ' + currentUser.name);
}

/**
 * Create default user profile
 */
function createDefaultUser() {
  return {
    id: 'user_' + Date.now(),
    name: 'Runner',
    teamId: null,
    leagueIds: ['league_global'],
    stats: {
      totalRuns: 0,
      totalDistance: 0,
      escapedCount: 0,
      caughtCount: 0,
      composureScore: 0,
      bestStreak: 0,
      currentStreak: 0
    },
    premium: false
  };
}

/**
 * Get current user profile
 */
function getUser() {
  return currentUser;
}

/**
 * Update user stats after a run
 */
function recordRun(runData) {
  if (!currentUser) return;

  currentUser.stats.totalRuns++;
  currentUser.stats.totalDistance += runData.distance || 0;

  if (runData.escaped) {
    currentUser.stats.escapedCount++;
    currentUser.stats.currentStreak++;
    if (currentUser.stats.currentStreak > currentUser.stats.bestStreak) {
      currentUser.stats.bestStreak = currentUser.stats.currentStreak;
    }
  } else {
    currentUser.stats.caughtCount++;
    currentUser.stats.currentStreak = 0;
  }

  // Calculate composure score (% of time on target pace)
  if (runData.composure !== undefined) {
    var totalRuns = currentUser.stats.totalRuns;
    currentUser.stats.composureScore =
      ((currentUser.stats.composureScore * (totalRuns - 1)) + runData.composure) / totalRuns;
  }

  // Save to local storage
  localStorage.setItem(STORAGE_USER_PROFILE, JSON.stringify(currentUser));

  console.log('Run recorded. Escaped: ' + runData.escaped + ', Composure: ' + runData.composure);
  return currentUser.stats;
}

/**
 * Get available leagues
 */
function getLeagues() {
  return Promise.resolve(MOCK_LEAGUES);
}

/**
 * Get league by ID
 */
function getLeague(leagueId) {
  var league = MOCK_LEAGUES.find(function (l) { return l.id === leagueId; });
  return Promise.resolve(league || null);
}

/**
 * Get team by ID
 */
function getTeam(teamId) {
  return Promise.resolve(MOCK_TEAMS[teamId] || null);
}

/**
 * Get leaderboard for a league
 */
function getLeaderboard(leagueId) {
  // Return mock leaderboard with current user included
  var leaderboard = MOCK_LEADERBOARD.slice();

  // Update "You" entry with actual stats
  if (currentUser) {
    var youIndex = leaderboard.findIndex(function (e) { return e.name === 'You'; });
    if (youIndex >= 0) {
      leaderboard[youIndex] = {
        rank: youIndex + 1,
        name: currentUser.name,
        composureScore: Math.round(currentUser.stats.composureScore * 10) / 10,
        escapedCount: currentUser.stats.escapedCount,
        caughtCount: currentUser.stats.caughtCount
      };
    }
  }

  return Promise.resolve(leaderboard);
}

/**
 * Join a team
 */
function joinTeam(teamId) {
  if (!currentUser) return Promise.reject('Not logged in');

  currentUser.teamId = teamId;
  localStorage.setItem(STORAGE_USER_PROFILE, JSON.stringify(currentUser));

  return Promise.resolve(MOCK_TEAMS[teamId]);
}

/**
 * Leave current team
 */
function leaveTeam() {
  if (!currentUser) return Promise.reject('Not logged in');

  currentUser.teamId = null;
  localStorage.setItem(STORAGE_USER_PROFILE, JSON.stringify(currentUser));

  return Promise.resolve();
}

/**
 * Check if user has premium
 */
function isPremium() {
  return currentUser ? currentUser.premium : false;
}

/**
 * Simulate premium upgrade (for testing)
 */
function setPremium(enabled) {
  if (currentUser) {
    currentUser.premium = enabled;
    localStorage.setItem(STORAGE_USER_PROFILE, JSON.stringify(currentUser));
  }
}

/**
 * Simulate "Segment Lost" notification
 * Returns info about who beat your segment
 */
function checkSegmentAlerts() {
  // Simulate random alerts (10% chance)
  if (Math.random() < 0.1) {
    return {
      type: 'segment_lost',
      segmentName: 'Elm Street Sprint',
      rivalName: 'Sarah C.',
      timeDiff: -3 // seconds faster
    };
  }
  return null;
}

/**
 * Clear all mock data (reset)
 */
function reset() {
  localStorage.removeItem(STORAGE_USER_PROFILE);
  localStorage.removeItem(STORAGE_LEAGUES);
  localStorage.removeItem(STORAGE_ACTIVITIES);
  currentUser = createDefaultUser();
}

module.exports = {
  init: init,
  getUser: getUser,
  recordRun: recordRun,
  getLeagues: getLeagues,
  getLeague: getLeague,
  getTeam: getTeam,
  getLeaderboard: getLeaderboard,
  joinTeam: joinTeam,
  leaveTeam: leaveTeam,
  isPremium: isPremium,
  setPremium: setPremium,
  checkSegmentAlerts: checkSegmentAlerts,
  reset: reset
};
