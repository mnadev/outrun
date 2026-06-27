/**
 * watch_commands.js - Handle incoming AppMessage payloads from the watch
 */

var Commands = {
  START: 1,
  STOP: 2,
  PAUSE: 3,
  RESUME: 4
};

// Raw AppMessage keys, matching comm.c KEY_* and index.js Keys. The C app
// deliberately uses raw integer keys 0..15 on both sides (bypassing the SDK's
// MESSAGE_KEY_* which are based at 10000), so a watch->phone message arrives
// keyed by these integers (e.g. e.payload[3]) -- NOT by name (e.payload.COMMAND
// would be undefined). Read the integer key; the name is also accepted for
// forward-compatibility and tests.
var MessageKeys = {
  TARGET_PACE: 1,
  COMMAND: 3
};

function readKey(payload, numericKey, name) {
  if (payload[numericKey] !== undefined) {
    return payload[numericKey];
  }
  if (payload[name] !== undefined) {
    return payload[name];
  }
  return undefined;
}

function applyWatchPayload(payload, handlers) {
  if (!payload) {
    return;
  }

  var command = readKey(payload, MessageKeys.COMMAND, 'COMMAND');
  if (command !== undefined) {
    switch (command) {
      case Commands.START:
        handlers.startTracking();
        break;
      case Commands.STOP:
        handlers.stopTracking();
        break;
      case Commands.PAUSE:
        handlers.pauseTracking();
        break;
      case Commands.RESUME:
        handlers.resumeTracking();
        break;
    }
  }

  var targetPace = readKey(payload, MessageKeys.TARGET_PACE, 'TARGET_PACE');
  if (targetPace !== undefined) {
    handlers.setTargetPace(targetPace);
  }
}

module.exports = {
  Commands: Commands,
  MessageKeys: MessageKeys,
  applyWatchPayload: applyWatchPayload
};
