/**
 * server_client.test.js - Tests for Server API Client module
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

// Mock XMLHttpRequest
class MockXMLHttpRequest {
  constructor() {
    this.method = null;
    this.url = null;
    this.headers = {};
    this.body = null;
    this.status = 200;
    this.responseText = '{}';
    this.onload = null;
    this.onerror = null;
  }

  open(method, url) {
    this.method = method;
    this.url = url;
  }

  setRequestHeader(key, value) {
    this.headers[key] = value;
  }

  send(body) {
    this.body = body;
    // Simulate async response
    setTimeout(() => {
      if (this.onload) this.onload();
    }, 0);
  }
}

global.XMLHttpRequest = MockXMLHttpRequest;

const ServerClient = require('../server_client');

describe('ServerClient', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  describe('getDeviceId', () => {
    it('returns null initially', () => {
      expect(ServerClient.getDeviceId()).toBeNull();
    });

    it('returns stored device ID', () => {
      localStorage.setItem('outrun_device_id', 'device123');
      expect(ServerClient.getDeviceId()).toBe('device123');
    });
  });

  describe('getPairingCode', () => {
    it('returns null initially', () => {
      expect(ServerClient.getPairingCode()).toBeNull();
    });

    it('returns stored pairing code', () => {
      localStorage.setItem('outrun_pairing_code', 'A7X3K2');
      expect(ServerClient.getPairingCode()).toBe('A7X3K2');
    });
  });

  describe('getIsPaired', () => {
    it('returns false initially', () => {
      expect(ServerClient.getIsPaired()).toBe(false);
    });
  });

  describe('getUserSettings', () => {
    it('returns null initially', () => {
      expect(ServerClient.getUserSettings()).toBeNull();
    });
  });

  describe('setServerUrl', () => {
    it('stores the server URL', () => {
      ServerClient.setServerUrl('https://custom.outrun.app');
      expect(localStorage.getItem('outrun_server_url')).toBe('https://custom.outrun.app');
    });
  });

  describe('init', () => {
    it('returns a promise', () => {
      const result = ServerClient.init();
      expect(result).toBeInstanceOf(Promise);
    });
  });

  describe('saveRun', () => {
    it('returns a promise', () => {
      localStorage.setItem('outrun_device_id', 'device123');
      const result = ServerClient.saveRun({
        distance: 5000,
        elapsed: 1800,
        avgPace: 360,
        composure: 85,
        escaped: true
      });
      expect(result).toBeInstanceOf(Promise);
    });

    it('rejects without device ID', () => {
      return ServerClient.saveRun({}).catch(err => {
        expect(err).toBe('No device ID');
      });
    });
  });

  describe('syncToStrava', () => {
    it('returns a promise', () => {
      localStorage.setItem('outrun_device_id', 'device123');
      const result = ServerClient.syncToStrava('run123');
      expect(result).toBeInstanceOf(Promise);
    });

    it('rejects without device ID', () => {
      return ServerClient.syncToStrava('run123').catch(err => {
        expect(err).toBe('No device ID');
      });
    });
  });

  describe('checkPairingStatus', () => {
    it('rejects without device ID', () => {
      return ServerClient.checkPairingStatus().catch(err => {
        expect(err).toBe('No device ID');
      });
    });
  });
});
