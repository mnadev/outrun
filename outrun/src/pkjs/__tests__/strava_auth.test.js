/**
 * strava_auth.test.js - Tests for Strava OAuth module
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

const StravaAuth = require('../strava_auth');

describe('StravaAuth', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  describe('isAuthenticated', () => {
    it('returns false with no tokens', () => {
      expect(StravaAuth.isAuthenticated()).toBe(false);
    });

    it('returns true with valid tokens', () => {
      localStorage.setItem('strava_access_token', 'test_token');
      localStorage.setItem('strava_token_expires', (Date.now() + 3600000).toString());
      expect(StravaAuth.isAuthenticated()).toBe(true);
    });

    it('returns false with expired token', () => {
      localStorage.setItem('strava_access_token', 'test_token');
      localStorage.setItem('strava_token_expires', (Date.now() - 1000).toString());
      expect(StravaAuth.isAuthenticated()).toBe(false);
    });
  });

  describe('getAccessToken', () => {
    it('returns null with no token', () => {
      expect(StravaAuth.getAccessToken()).toBeNull();
    });

    it('returns token when stored', () => {
      localStorage.setItem('strava_access_token', 'my_token');
      localStorage.setItem('strava_token_expires', (Date.now() + 3600000).toString());
      expect(StravaAuth.getAccessToken()).toBe('my_token');
    });
  });

  describe('getAuthorizationUrl', () => {
    it('returns a valid Strava URL', () => {
      const url = StravaAuth.getAuthorizationUrl();
      expect(url).toContain('strava.com/oauth/authorize');
      expect(url).toContain('client_id=');
      expect(url).toContain('scope=');
    });

    it('includes required scopes', () => {
      const url = StravaAuth.getAuthorizationUrl();
      expect(url).toContain('read');
      expect(url).toContain('activity');
    });
  });

  describe('handleAuthCallback', () => {
    it('stores tokens from callback data', () => {
      const callbackData = {
        access_token: 'new_access_token',
        refresh_token: 'new_refresh_token',
        expires_at: Math.floor(Date.now() / 1000) + 3600
      };

      StravaAuth.handleAuthCallback(callbackData);

      expect(localStorage.getItem('strava_access_token')).toBe('new_access_token');
      expect(localStorage.getItem('strava_refresh_token')).toBe('new_refresh_token');
    });
  });

  describe('logout', () => {
    it('clears all tokens', () => {
      localStorage.setItem('strava_access_token', 'token');
      localStorage.setItem('strava_refresh_token', 'refresh');
      localStorage.setItem('strava_token_expires', '12345');

      StravaAuth.logout();

      expect(localStorage.getItem('strava_access_token')).toBeNull();
      expect(localStorage.getItem('strava_refresh_token')).toBeNull();
      expect(localStorage.getItem('strava_token_expires')).toBeNull();
    });
  });

  describe('needsRefresh', () => {
    it('returns true when token expires soon', () => {
      localStorage.setItem('strava_access_token', 'token');
      localStorage.setItem('strava_token_expires', (Date.now() + 60000).toString()); // 1 min
      expect(StravaAuth.needsRefresh()).toBe(true);
    });

    it('returns false when token is fresh', () => {
      localStorage.setItem('strava_access_token', 'token');
      localStorage.setItem('strava_token_expires', (Date.now() + 3600000).toString()); // 1 hour
      expect(StravaAuth.needsRefresh()).toBe(false);
    });
  });
});
