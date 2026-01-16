/**
 * strava_auth.js - Strava OAuth 2.0 authentication
 * 
 * Handles the OAuth flow for Strava:
 * 1. Redirect user to Strava authorization page
 * 2. Receive authorization code via config page callback
 * 3. Exchange code for access/refresh tokens
 * 4. Store tokens in localStorage
 * 5. Auto-refresh expired tokens
 */

// Strava API endpoints
var STRAVA_AUTH_URL = 'https://www.strava.com/oauth/authorize';
var STRAVA_TOKEN_URL = 'https://www.strava.com/oauth/token';

// These would normally come from your backend or be stored securely
// For a real app, you'd proxy token exchange through your backend
var CLIENT_ID = 'YOUR_STRAVA_CLIENT_ID';       // Set via config
var CLIENT_SECRET = 'YOUR_STRAVA_CLIENT_SECRET'; // Set via config (or backend)
var REDIRECT_URI = 'https://outrun.app/strava-callback'; // Your config page

// Storage keys
var STORAGE_ACCESS_TOKEN = 'strava_access_token';
var STORAGE_REFRESH_TOKEN = 'strava_refresh_token';
var STORAGE_EXPIRES_AT = 'strava_expires_at';
var STORAGE_ATHLETE_ID = 'strava_athlete_id';

/**
 * Check if user is authenticated with Strava
 */
function isAuthenticated() {
  var token = localStorage.getItem(STORAGE_ACCESS_TOKEN);
  var expiresAt = localStorage.getItem(STORAGE_EXPIRES_AT);

  if (!token || !expiresAt) {
    return false;
  }

  // Check if token is expired
  var now = Math.floor(Date.now() / 1000);
  if (now >= parseInt(expiresAt)) {
    // Token expired, try to refresh
    return false;
  }

  return true;
}

/**
 * Get the current access token (refreshing if needed)
 * @returns {Promise<string|null>}
 */
function getAccessToken() {
  return new Promise(function (resolve, reject) {
    var token = localStorage.getItem(STORAGE_ACCESS_TOKEN);
    var expiresAt = localStorage.getItem(STORAGE_EXPIRES_AT);
    var refreshToken = localStorage.getItem(STORAGE_REFRESH_TOKEN);

    if (!token) {
      resolve(null);
      return;
    }

    var now = Math.floor(Date.now() / 1000);
    if (now < parseInt(expiresAt)) {
      // Token still valid
      resolve(token);
      return;
    }

    // Token expired, refresh it
    if (!refreshToken) {
      resolve(null);
      return;
    }

    refreshAccessToken(refreshToken)
      .then(function (newToken) {
        resolve(newToken);
      })
      .catch(function (err) {
        console.log('Failed to refresh token: ' + err);
        resolve(null);
      });
  });
}

/**
 * Refresh the access token
 * @param {string} refreshToken
 * @returns {Promise<string>}
 */
function refreshAccessToken(refreshToken) {
  return new Promise(function (resolve, reject) {
    var req = new XMLHttpRequest();
    req.open('POST', STRAVA_TOKEN_URL, true);
    req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');

    req.onload = function () {
      if (req.status === 200) {
        var response = JSON.parse(req.responseText);
        storeTokens(response);
        resolve(response.access_token);
      } else {
        reject('Token refresh failed: ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error during token refresh');
    };

    var params = 'client_id=' + CLIENT_ID +
      '&client_secret=' + CLIENT_SECRET +
      '&grant_type=refresh_token' +
      '&refresh_token=' + refreshToken;

    req.send(params);
  });
}

/**
 * Exchange authorization code for tokens
 * @param {string} code Authorization code from OAuth callback
 * @returns {Promise<Object>}
 */
function exchangeCodeForTokens(code) {
  return new Promise(function (resolve, reject) {
    var req = new XMLHttpRequest();
    req.open('POST', STRAVA_TOKEN_URL, true);
    req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');

    req.onload = function () {
      if (req.status === 200) {
        var response = JSON.parse(req.responseText);
        storeTokens(response);
        resolve(response);
      } else {
        reject('Token exchange failed: ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error during token exchange');
    };

    var params = 'client_id=' + CLIENT_ID +
      '&client_secret=' + CLIENT_SECRET +
      '&code=' + code +
      '&grant_type=authorization_code';

    req.send(params);
  });
}

/**
 * Store tokens in localStorage
 * @param {Object} tokenResponse
 */
function storeTokens(tokenResponse) {
  localStorage.setItem(STORAGE_ACCESS_TOKEN, tokenResponse.access_token);
  localStorage.setItem(STORAGE_REFRESH_TOKEN, tokenResponse.refresh_token);
  localStorage.setItem(STORAGE_EXPIRES_AT, tokenResponse.expires_at.toString());

  if (tokenResponse.athlete && tokenResponse.athlete.id) {
    localStorage.setItem(STORAGE_ATHLETE_ID, tokenResponse.athlete.id.toString());
  }

  console.log('Strava tokens stored, expires at: ' + new Date(tokenResponse.expires_at * 1000));
}

/**
 * Clear all stored tokens (logout)
 */
function clearTokens() {
  localStorage.removeItem(STORAGE_ACCESS_TOKEN);
  localStorage.removeItem(STORAGE_REFRESH_TOKEN);
  localStorage.removeItem(STORAGE_EXPIRES_AT);
  localStorage.removeItem(STORAGE_ATHLETE_ID);
}

/**
 * Get the OAuth authorization URL
 * @returns {string}
 */
function getAuthorizationUrl() {
  var scope = 'read,activity:read_all';
  var responseType = 'code';

  return STRAVA_AUTH_URL +
    '?client_id=' + CLIENT_ID +
    '&redirect_uri=' + encodeURIComponent(REDIRECT_URI) +
    '&response_type=' + responseType +
    '&scope=' + encodeURIComponent(scope) +
    '&approval_prompt=auto';
}

/**
 * Get athlete ID
 */
function getAthleteId() {
  return localStorage.getItem(STORAGE_ATHLETE_ID);
}

/**
 * Set client credentials (from config)
 */
function setClientCredentials(clientId, clientSecret) {
  CLIENT_ID = clientId;
  CLIENT_SECRET = clientSecret;
}

module.exports = {
  isAuthenticated: isAuthenticated,
  getAccessToken: getAccessToken,
  exchangeCodeForTokens: exchangeCodeForTokens,
  clearTokens: clearTokens,
  getAuthorizationUrl: getAuthorizationUrl,
  getAthleteId: getAthleteId,
  setClientCredentials: setClientCredentials
};
