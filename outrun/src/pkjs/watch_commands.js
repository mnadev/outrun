/**
 * watch_commands.js - Handle incoming AppMessage payloads from the watch
 */

var Commands = {
  START: 1,
  STOP: 2,
  PAUSE: 3,
  RESUME: 4
};

function applyWatchPayload(payload, handlers) {
  if (payload.COMMAND !== undefined) {
    switch (payload.COMMAND) {
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

  if (payload.TARGET_PACE !== undefined) {
    handlers.setTargetPace(payload.TARGET_PACE);
  }
}

module.exports = {
  Commands: Commands,
  applyWatchPayload: applyWatchPayload
};
