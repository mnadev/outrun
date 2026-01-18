/**
 * strava_auth.test.js - Tests for Strava OAuth module
 */

describe('StravaAuth', () => {
  let StravaAuth;

  beforeEach(() => {
    jest.resetModules();
    StravaAuth = require('../strava_auth');
  });

  describe('isAuthenticated', () => {
    it('returns false with no tokens', () => {
      expect(StravaAuth.isAuthenticated()).toBe(false);
    });

    it('returns true with valid tokens', () => {
      // Uses strava_expires_at (unix timestamp), not strava_token_expires
      localStorage.setItem('strava_access_token', 'test_token');
      localStorage.setItem('strava_expires_at', (Math.floor(Date.now() / 1000) + 3600).toString());
      expect(StravaAuth.isAuthenticated()).toBe(true);
    });

    it('returns false with expired token', () => {
      localStorage.setItem('strava_access_token', 'test_token');
      localStorage.setItem('strava_expires_at', (Math.floor(Date.now() / 1000) - 100).toString());
      expect(StravaAuth.isAuthenticated()).toBe(false);
    });
  });

  describe('getAccessToken', () => {
    it('resolves to null with no token', async () => {
      const token = await StravaAuth.getAccessToken();
      expect(token).toBeNull();
    });

    it('resolves to token when stored and valid', async () => {
      localStorage.setItem('strava_access_token', 'my_token');
      localStorage.setItem('strava_expires_at', (Math.floor(Date.now() / 1000) + 3600).toString());
      const token = await StravaAuth.getAccessToken();
      expect(token).toBe('my_token');
    });
  });

  describe('getAuthorizationUrl', () => {
    it('returns a valid Strava URL', () => {
      const url = StravaAuth.getAuthorizationUrl();
      expect(url).toContain('strava.com/oauth/authorize');
    });

    it('includes client_id', () => {
      const url = StravaAuth.getAuthorizationUrl();
      expect(url).toContain('client_id=');
    });

    it('includes scope', () => {
      const url = StravaAuth.getAuthorizationUrl();
      expect(url).toContain('scope=');
    });
  });

  describe('clearTokens', () => {
    it('clears all tokens', () => {
      localStorage.setItem('strava_access_token', 'token');
      localStorage.setItem('strava_refresh_token', 'refresh');
      localStorage.setItem('strava_expires_at', '12345');

      StravaAuth.clearTokens();

      expect(localStorage.getItem('strava_access_token')).toBeNull();
      expect(localStorage.getItem('strava_refresh_token')).toBeNull();
      expect(localStorage.getItem('strava_expires_at')).toBeNull();
    });
  });

  describe('setClientCredentials', () => {
    it('sets the client ID and secret', () => {
      // This just tests that the function exists and runs without error
      expect(() => {
        StravaAuth.setClientCredentials('test_id', 'test_secret');
      }).not.toThrow();
    });
  });

  describe('getAthleteId', () => {
    it('returns null with no athlete ID', () => {
      expect(StravaAuth.getAthleteId()).toBeNull();
    });

    it('returns stored athlete ID', () => {
      localStorage.setItem('strava_athlete_id', '12345');
      expect(StravaAuth.getAthleteId()).toBe('12345');
    });
  });
});
