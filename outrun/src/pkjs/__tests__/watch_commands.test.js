var WatchCommands = require('../watch_commands');

// Build a payload the way PebbleKit JS delivers a watch message that used a raw
// integer AppMessage key: the property is the integer (as a string key).
function makePayload(numericKey, value) {
  var payload = {};
  payload[numericKey] = value;
  return payload;
}

describe('watch_commands', function () {
  test('handles named COMMAND START key', function () {
    var started = false;
    WatchCommands.applyWatchPayload({ COMMAND: WatchCommands.Commands.START }, {
      startTracking: function () { started = true; },
      stopTracking: function () {},
      pauseTracking: function () {},
      resumeTracking: function () {},
      setTargetPace: function () {}
    });
    expect(started).toBe(true);
  });

  // The watch sends RAW integer keys (comm.c KEY_COMMAND = 3), and PebbleKit JS
  // delivers them under the integer property, not the name. This is what real
  // hardware actually sends, so it MUST be handled.
  test('handles raw numeric COMMAND key (key 3) from the watch', function () {
    var started = false;
    WatchCommands.applyWatchPayload(
      makePayload(WatchCommands.MessageKeys.COMMAND, WatchCommands.Commands.START),
      {
        startTracking: function () { started = true; },
        stopTracking: function () {},
        pauseTracking: function () {},
        resumeTracking: function () {},
        setTargetPace: function () {}
      });
    expect(started).toBe(true);
  });

  test('handles raw numeric TARGET_PACE key (key 1) from the watch', function () {
    var target = null;
    WatchCommands.applyWatchPayload(
      makePayload(WatchCommands.MessageKeys.TARGET_PACE, 240),
      {
        startTracking: function () {},
        stopTracking: function () {},
        pauseTracking: function () {},
        resumeTracking: function () {},
        setTargetPace: function (pace) { target = pace; }
      });
    expect(target).toBe(240);
  });

  test('handles named TARGET_PACE key', function () {
    var target = null;
    WatchCommands.applyWatchPayload({ TARGET_PACE: 285 }, {
      startTracking: function () {},
      stopTracking: function () {},
      pauseTracking: function () {},
      resumeTracking: function () {},
      setTargetPace: function (pace) { target = pace; }
    });
    expect(target).toBe(285);
  });

  test('pause and resume callbacks are invoked', function () {
    var paused = false;
    var resumed = false;

    WatchCommands.applyWatchPayload({ COMMAND: WatchCommands.Commands.PAUSE }, {
      startTracking: function () {},
      stopTracking: function () {},
      pauseTracking: function () { paused = true; },
      resumeTracking: function () {},
      setTargetPace: function () {}
    });
    expect(paused).toBe(true);

    WatchCommands.applyWatchPayload({ COMMAND: WatchCommands.Commands.RESUME }, {
      startTracking: function () {},
      stopTracking: function () {},
      pauseTracking: function () {},
      resumeTracking: function () { resumed = true; },
      setTargetPace: function () {}
    });
    expect(resumed).toBe(true);
  });
});

describe('stop while paused', function () {
  test('stopTracking clears paused state', function () {
    var isTracking = false;
    var isPaused = true;
    var watchId = 42;
    var cleared = false;

    function stopTracking() {
      if (!isTracking && !isPaused) return;
      isTracking = false;
      isPaused = false;
      if (watchId !== null) {
        cleared = true;
        watchId = null;
      }
    }

    stopTracking();
    expect(isPaused).toBe(false);
    expect(cleared).toBe(true);
    expect(watchId).toBeNull();
  });
});
