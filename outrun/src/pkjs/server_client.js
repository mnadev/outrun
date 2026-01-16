/**
 * server_client.js - Client for Outrun Server API
 * 
 * Replaces mock_backend.js with real server calls.
 */

// Server URL - configurable via localStorage or hardcoded
var SERVER_URL = localStorage.getItem('outrun_server_url') || 'https://outrun.app';

// Device state
var deviceId = localStorage.getItem('outrun_device_id');
var pairingCode = localStorage.getItem('outrun_pairing_code');
var isPaired = false;
var userSettings = null;

/**
 * Initialize - register device if needed
 */
function init() {
  return new Promise(function (resolve, reject) {
    if (deviceId) {
      // Check if device is paired
      checkPairingStatus().then(function (status) {
        isPaired = status.paired;
        userSettings = status.user;
        console.log('Device status: ' + (isPaired ? 'paired' : 'not paired'));
        resolve({ isPaired: isPaired, pairingCode: pairingCode });
      }).catch(function (err) {
        console.log('Failed to check pairing: ' + err);
        resolve({ isPaired: false, pairingCode: pairingCode });
      });
    } else {
      // Register new device
      registerDevice().then(function (result) {
        deviceId = result.deviceId;
        pairingCode = result.pairingCode;
        localStorage.setItem('outrun_device_id', deviceId);
        localStorage.setItem('outrun_pairing_code', pairingCode);
        console.log('Device registered. Pairing code: ' + pairingCode);
        resolve({ isPaired: false, pairingCode: pairingCode });
      }).catch(function (err) {
        console.log('Failed to register device: ' + err);
        reject(err);
      });
    }
  });
}

/**
 * Register a new device
 */
function registerDevice() {
  return new Promise(function (resolve, reject) {
    var req = new XMLHttpRequest();
    req.open('POST', SERVER_URL + '/api/device/register', true);
    req.setRequestHeader('Content-Type', 'application/json');

    req.onload = function () {
      if (req.status === 200) {
        var data = JSON.parse(req.responseText);
        if (data.success) {
          resolve(data);
        } else {
          reject(data.error);
        }
      } else {
        reject('HTTP ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error');
    };

    req.send('{}');
  });
}

/**
 * Check if device is paired to a user
 */
function checkPairingStatus() {
  return new Promise(function (resolve, reject) {
    if (!deviceId) {
      reject('No device ID');
      return;
    }

    var req = new XMLHttpRequest();
    req.open('GET', SERVER_URL + '/api/device/register?deviceId=' + deviceId, true);

    req.onload = function () {
      if (req.status === 200) {
        var data = JSON.parse(req.responseText);
        if (data.success) {
          isPaired = data.paired;
          userSettings = data.user;
          resolve(data);
        } else {
          reject(data.error);
        }
      } else {
        reject('HTTP ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error');
    };

    req.send();
  });
}

/**
 * Save a completed run to the server
 */
function saveRun(runData) {
  return new Promise(function (resolve, reject) {
    if (!deviceId) {
      reject('No device ID');
      return;
    }

    var req = new XMLHttpRequest();
    req.open('POST', SERVER_URL + '/api/run/save', true);
    req.setRequestHeader('Content-Type', 'application/json');

    req.onload = function () {
      if (req.status === 200) {
        var data = JSON.parse(req.responseText);
        if (data.success) {
          console.log('Run saved: ' + data.runId);
          resolve(data);
        } else {
          reject(data.error);
        }
      } else {
        reject('HTTP ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error');
    };

    var payload = {
      deviceId: deviceId,
      distance: runData.distance || 0,
      duration: runData.elapsed || 0,
      avgPace: runData.avgPace || 0,
      composure: runData.composure || 0,
      escaped: runData.escaped || false,
      closeCalls: runData.closeCalls || 0,
      dangerTime: runData.dangerTime || 0,
      gpxData: runData.gpxData || null,
      theme: runData.theme || 'classic'
    };

    req.send(JSON.stringify(payload));
  });
}

/**
 * Sync a run to Strava
 */
function syncToStrava(runId) {
  return new Promise(function (resolve, reject) {
    if (!deviceId) {
      reject('No device ID');
      return;
    }

    var req = new XMLHttpRequest();
    req.open('POST', SERVER_URL + '/api/strava/sync', true);
    req.setRequestHeader('Content-Type', 'application/json');

    req.onload = function () {
      if (req.status === 200) {
        var data = JSON.parse(req.responseText);
        if (data.success) {
          console.log('Synced to Strava: ' + data.stravaUrl);
          resolve(data);
        } else {
          reject(data.error);
        }
      } else {
        reject('HTTP ' + req.status);
      }
    };

    req.onerror = function () {
      reject('Network error');
    };

    req.send(JSON.stringify({ runId: runId, deviceId: deviceId }));
  });
}

/**
 * Get current pairing code
 */
function getPairingCode() {
  return pairingCode;
}

/**
 * Get device ID
 */
function getDeviceId() {
  return deviceId;
}

/**
 * Check if paired
 */
function getIsPaired() {
  return isPaired;
}

/**
 * Get user settings (if paired)
 */
function getUserSettings() {
  return userSettings;
}

/**
 * Set server URL
 */
function setServerUrl(url) {
  SERVER_URL = url;
  localStorage.setItem('outrun_server_url', url);
}

module.exports = {
  init: init,
  registerDevice: registerDevice,
  checkPairingStatus: checkPairingStatus,
  saveRun: saveRun,
  syncToStrava: syncToStrava,
  getPairingCode: getPairingCode,
  getDeviceId: getDeviceId,
  getIsPaired: getIsPaired,
  getUserSettings: getUserSettings,
  setServerUrl: setServerUrl
};
