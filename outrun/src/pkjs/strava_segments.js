/**
 * strava_segments.js - Strava segment management
 * 
 * Handles:
 * - Fetching starred segments from Strava API
 * - Caching segments locally
 * - Detecting when user enters/exits a segment
 * - Getting segment efforts for comparison
 */

var StravaAuth = require('./strava_auth');

// Strava API endpoints
var STRAVA_API_BASE = 'https://www.strava.com/api/v3';

// Storage
var STORAGE_SEGMENTS = 'strava_segments';
var cachedSegments = [];

/**
 * Segment object structure
 * @typedef {Object} Segment
 * @property {number} id - Strava segment ID
 * @property {string} name - Segment name
 * @property {number} distance - Distance in meters
 * @property {Object} start_latlng - [lat, lng]
 * @property {Object} end_latlng - [lat, lng]
 * @property {number} pr_time - Personal record time in seconds
 * @property {string} rival_name - Name of rival to beat
 * @property {number} rival_time - Rival's time to beat
 */

/**
 * Fetch starred segments from Strava
 * @returns {Promise<Segment[]>}
 */
function fetchStarredSegments() {
  return new Promise(function (resolve, reject) {
    StravaAuth.getAccessToken().then(function (token) {
      if (!token) {
        reject('Not authenticated with Strava');
        return;
      }

      var req = new XMLHttpRequest();
      req.open('GET', STRAVA_API_BASE + '/segments/starred', true);
      req.setRequestHeader('Authorization', 'Bearer ' + token);

      req.onload = function () {
        if (req.status === 200) {
          var segments = JSON.parse(req.responseText);
          cachedSegments = segments.map(function (s) {
            return {
              id: s.id,
              name: s.name,
              distance: s.distance,
              start_latlng: s.start_latlng,
              end_latlng: s.end_latlng,
              pr_time: s.athlete_pr_effort ? s.athlete_pr_effort.elapsed_time : null,
              rival_name: null,
              rival_time: null
            };
          });

          // Cache to localStorage
          localStorage.setItem(STORAGE_SEGMENTS, JSON.stringify(cachedSegments));
          console.log('Fetched ' + cachedSegments.length + ' starred segments');
          resolve(cachedSegments);
        } else if (req.status === 401) {
          reject('Strava auth expired');
        } else {
          reject('Failed to fetch segments: ' + req.status);
        }
      };

      req.onerror = function () {
        reject('Network error fetching segments');
      };

      req.send();
    }).catch(reject);
  });
}

/**
 * Load cached segments from localStorage
 * @returns {Segment[]}
 */
function loadCachedSegments() {
  var stored = localStorage.getItem(STORAGE_SEGMENTS);
  if (stored) {
    cachedSegments = JSON.parse(stored);
  }
  return cachedSegments;
}

/**
 * Get all segments (cached or fetch)
 * @param {boolean} forceRefresh Force fetch from API
 * @returns {Promise<Segment[]>}
 */
function getSegments(forceRefresh) {
  if (!forceRefresh && cachedSegments.length > 0) {
    return Promise.resolve(cachedSegments);
  }

  var cached = loadCachedSegments();
  if (!forceRefresh && cached.length > 0) {
    return Promise.resolve(cached);
  }

  return fetchStarredSegments();
}

/**
 * Calculate distance between two GPS points (meters)
 */
function haversineDistance(lat1, lng1, lat2, lng2) {
  var R = 6371000;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLng = (lng2 - lng1) * Math.PI / 180;

  var a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
    Math.sin(dLng / 2) * Math.sin(dLng / 2);

  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

/**
 * Check if a position is near a segment start
 * @param {number} lat Current latitude
 * @param {number} lng Current longitude
 * @param {number} threshold Distance threshold in meters
 * @returns {Segment|null} The segment if near its start, null otherwise
 */
function findNearbySegmentStart(lat, lng, threshold) {
  threshold = threshold || 50; // Default 50m

  for (var i = 0; i < cachedSegments.length; i++) {
    var seg = cachedSegments[i];
    if (seg.start_latlng && seg.start_latlng.length >= 2) {
      var dist = haversineDistance(lat, lng, seg.start_latlng[0], seg.start_latlng[1]);
      if (dist < threshold) {
        return seg;
      }
    }
  }
  return null;
}

/**
 * Check if a position is near a segment end
 * @param {Segment} segment The segment to check
 * @param {number} lat Current latitude
 * @param {number} lng Current longitude
 * @param {number} threshold Distance threshold in meters
 * @returns {boolean}
 */
function isNearSegmentEnd(segment, lat, lng, threshold) {
  threshold = threshold || 50;

  if (segment.end_latlng && segment.end_latlng.length >= 2) {
    var dist = haversineDistance(lat, lng, segment.end_latlng[0], segment.end_latlng[1]);
    return dist < threshold;
  }
  return false;
}

/**
 * Fetch segment leaderboard to find a rival
 * @param {number} segmentId
 * @returns {Promise<Object>} Rival info {name, time}
 */
function fetchSegmentRival(segmentId) {
  return new Promise(function (resolve, reject) {
    StravaAuth.getAccessToken().then(function (token) {
      if (!token) {
        reject('Not authenticated');
        return;
      }

      // Get segment leaderboard filtered to following
      var url = STRAVA_API_BASE + '/segments/' + segmentId + '/leaderboard?following=true&per_page=5';

      var req = new XMLHttpRequest();
      req.open('GET', url, true);
      req.setRequestHeader('Authorization', 'Bearer ' + token);

      req.onload = function () {
        if (req.status === 200) {
          var data = JSON.parse(req.responseText);
          if (data.entries && data.entries.length > 0) {
            // Find the first entry that isn't the current user
            var athleteId = StravaAuth.getAthleteId();
            for (var i = 0; i < data.entries.length; i++) {
              if (data.entries[i].athlete_id.toString() !== athleteId) {
                resolve({
                  name: data.entries[i].athlete_name,
                  time: data.entries[i].elapsed_time
                });
                return;
              }
            }
          }
          // No rival found, use own PR
          resolve(null);
        } else {
          reject('Failed to fetch leaderboard');
        }
      };

      req.onerror = function () {
        reject('Network error');
      };

      req.send();
    }).catch(reject);
  });
}

/**
 * Clear cached segments
 */
function clearCache() {
  cachedSegments = [];
  localStorage.removeItem(STORAGE_SEGMENTS);
}

module.exports = {
  fetchStarredSegments: fetchStarredSegments,
  loadCachedSegments: loadCachedSegments,
  getSegments: getSegments,
  findNearbySegmentStart: findNearbySegmentStart,
  isNearSegmentEnd: isNearSegmentEnd,
  fetchSegmentRival: fetchSegmentRival,
  clearCache: clearCache
};
