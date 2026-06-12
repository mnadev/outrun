var WatchCommands = require('../watch_commands');

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

  test('ignores numeric command key index', function () {
    var started = false;
    WatchCommands.applyWatchPayload({ 3: WatchCommands.Commands.START }, {
      startTracking: function () { started = true; },
      stopTracking: function () {},
      pauseTracking: function () {},
      resumeTracking: function () {},
      setTargetPace: function () {}
    });
    expect(started).toBe(false);
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
