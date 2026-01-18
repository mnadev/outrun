/**
 * server_client.test.js - Tests for Server API Client module
 */

describe('ServerClient', () => {
  let ServerClient;

  beforeEach(() => {
    jest.resetModules();

    // Mock XMLHttpRequest
    global.XMLHttpRequest = class {
      constructor() {
        this.status = 200;
        this.responseText = '{"success": true}';
      }
      open() { }
      setRequestHeader() { }
      send() {
        setTimeout(() => { if (this.onload) this.onload(); }, 0);
      }
    };

    ServerClient = require('../server_client');
  });

  describe('getDeviceId', () => {
    it('returns null initially', () => {
      expect(ServerClient.getDeviceId()).toBeNull();
    });

    it('returns stored device ID', () => {
      localStorage.setItem('outrun_device_id', 'device123');
      // Re-require to pick up localStorage
      jest.resetModules();
      ServerClient = require('../server_client');
      expect(ServerClient.getDeviceId()).toBe('device123');
    });
  });

  describe('getPairingCode', () => {
    it('returns null initially', () => {
      expect(ServerClient.getPairingCode()).toBeNull();
    });

    it('returns stored pairing code', () => {
      localStorage.setItem('outrun_pairing_code', 'A7X3K2');
      jest.resetModules();
      ServerClient = require('../server_client');
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
    it('rejects without device ID', (done) => {
      ServerClient.saveRun({}).catch(err => {
        expect(err).toBe('No device ID');
        done();
      });
    });

    it('returns a promise with device ID', () => {
      localStorage.setItem('outrun_device_id', 'device123');
      jest.resetModules();
      ServerClient = require('../server_client');

      const result = ServerClient.saveRun({
        distance: 5000,
        elapsed: 1800
      });
      expect(result).toBeInstanceOf(Promise);
    });
  });

  describe('syncToStrava', () => {
    it('rejects without device ID', (done) => {
      ServerClient.syncToStrava('run123').catch(err => {
        expect(err).toBe('No device ID');
        done();
      });
    });
  });

  describe('checkPairingStatus', () => {
    it('rejects without device ID', (done) => {
      ServerClient.checkPairingStatus().catch(err => {
        expect(err).toBe('No device ID');
        done();
      });
    });
  });
});
