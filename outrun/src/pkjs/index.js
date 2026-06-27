/**
 * index.js - PebbleKit JS entry point
 * 
 * Handles:
 * - GPS location tracking
 * - Pace calculation
 * - Communication with the Pebble watch
 * - Strava OAuth and segment detection
 */

var PaceCalculator = require('./pace_calculator');
var PlanPresets = require('./plan_presets');
var WatchCommands = require('./watch_commands');
var StravaAuth = require('./strava_auth');
var StravaSegments = require('./strava_segments');
var ServerClient = require('./server_client');
var GhostRacing = require('./ghost_racing');

// State
var isTracking = false;
var isPaused = false;
var watchId = null;
var paceCalculator = new PaceCalculator();
var activeSegment = null;
var segmentStartTime = null;
var fullTrack = [];        // full GPS track for saving as a ghost
var lastGhostUpdate = 0;   // throttle rival gap updates to the watch

// Message keys (must match package.json)
var Keys = {
    CURRENT_PACE: 0,
    TARGET_PACE: 1,
    IS_RUNNING: 2,
    COMMAND: 3,
    SEGMENT_ACTIVE: 4,
    SEGMENT_NAME: 5,
    RIVAL_NAME: 6,
    RIVAL_TIME: 7,
    CONNECTED: 8,
    CURRENT_DISTANCE: 9,
    HEART_RATE: 10,
    PLAN_INDEX: 11,
    PLAN_NAME: 12,
    PLAN_SEG_COUNT: 13,
    PLAN_DATA: 14,
    PLAN_TOTAL: 15
};

// Commands from watch
var Commands = WatchCommands.Commands;

/**
 * Send pace data to the watch
 */
var debugSimulatorTimer = null;
var debugLat = 37.7749;
var debugLng = -122.4194;
var debugHr = 130;
var debugPaceOffset = 0;

function sendPaceUpdate(currentPace, targetPace) {
    var message = {};
    message[Keys.CURRENT_PACE] = currentPace;
    message[Keys.TARGET_PACE] = targetPace;
    message[Keys.IS_RUNNING] = isTracking ? 1 : 0;
    message[Keys.CURRENT_DISTANCE] = paceCalculator.getTotalDistance();

    Pebble.sendAppMessage(message,
        function () {
            console.log('Pace update sent: ' + currentPace + ' sec/km');
        },
        function (e) {
            console.log('Failed to send pace update: ' + e.error);
        }
    );
}

/**
 * Send connected status to watch
 */
function sendHeartRate(bpm) {
    var message = {};
    message[Keys.HEART_RATE] = bpm;

    Pebble.sendAppMessage(message,
        function () {
            console.log('HR update sent: ' + bpm);
        },
        function (e) {
            console.log('Failed to send HR update: ' + e.error);
        }
    );
}

function sendPlan(planIndex, plan, totalPlans) {
    var packed = PlanPresets.encodePlan(plan.segments);
    var message = {};
    message[Keys.PLAN_TOTAL] = planIndex === 0 ? totalPlans : undefined;
    message[Keys.PLAN_INDEX] = planIndex;
    message[Keys.PLAN_NAME] = plan.name;
    message[Keys.PLAN_SEG_COUNT] = plan.segments.length;
    message[Keys.PLAN_DATA] = packed;

    if (message[Keys.PLAN_TOTAL] === undefined) {
        delete message[Keys.PLAN_TOTAL];
    }

    return new Promise(function (resolve, reject) {
        Pebble.sendAppMessage(message,
            function () {
                console.log('Plan sent: ' + plan.name);
                resolve();
            },
            function (e) {
                reject(e.error);
            }
        );
    });
}

function sendAllPlans() {
    var plans = PlanPresets.getPresetPlans();
    var chain = Promise.resolve();

    plans.forEach(function (plan, index) {
        chain = chain.then(function () {
            return sendPlan(index, plan, plans.length);
        });
    });

    return chain;
}

function sendConnectedStatus() {
    var message = {};
    message[Keys.CONNECTED] = 1;

    Pebble.sendAppMessage(message,
        function () {
            console.log('Connected status sent to watch');
        },
        function (e) {
            console.log('Failed to send connected status: ' + e.error);
        }
    );
}

/**
 * Send segment alert to watch
 */
function sendSegmentAlert(segmentName, rivalName, rivalTime) {
    var message = {};
    message[Keys.SEGMENT_ACTIVE] = 1;
    message[Keys.SEGMENT_NAME] = segmentName || 'Segment';
    message[Keys.RIVAL_NAME] = rivalName || 'PR';
    message[Keys.RIVAL_TIME] = rivalTime || 0;

    Pebble.sendAppMessage(message,
        function () {
            console.log('Segment alert sent: ' + segmentName);
        },
        function (e) {
            console.log('Failed to send segment alert: ' + e.error);
        }
    );
}

/**
 * Send segment end to watch
 */
function sendSegmentEnd() {
    var message = {};
    message[Keys.SEGMENT_ACTIVE] = 0;

    Pebble.sendAppMessage(message);
}

/**
 * Race the most recent saved run (offline; no server needed).
 */
function startGhostRace() {
    var ghosts = GhostRacing.loadGhosts();
    if (!ghosts || ghosts.length === 0) {
        return;
    }
    if (GhostRacing.startRacing(ghosts[0].id)) {
        lastGhostUpdate = 0;
        var ghost = GhostRacing.getCurrentGhost();
        var name = ghost ? ghost.name : 'Ghost';
        sendSegmentAlert(name, name, 0); // rival appears -> watch jump scare
    }
}

/**
 * Send the rival gap (ahead/behind) to the watch, throttled.
 */
function updateGhostRace(lat, lng) {
    if (!GhostRacing.isRacing()) {
        return;
    }
    var now = Date.now();
    if (now - lastGhostUpdate < 5000) {
        return;
    }
    lastGhostUpdate = now;

    var pos = GhostRacing.getGhostPosition(lat, lng,
                                           paceCalculator.getTotalDistance());
    if (!pos) {
      return;
    }
    var ghost = GhostRacing.getCurrentGhost();
    var name = ghost ? ghost.name : 'Ghost';

    if (pos.ghostFinished) {
        sendSegmentAlert(name, name + ' done', 0);
        GhostRacing.stopRacing();
        return;
    }

    // timeDiff units are loosely defined in the module, so show a robust
    // ahead/behind indicator rather than a possibly-miscalibrated number.
    var label = name + (pos.timeDiff >= 0 ? ' ahead' : ' behind');
    sendSegmentAlert(name, label, pos.timeDiff);
}

/**
 * Handle GPS position update
 */
function handlePosition(position) {
    if (!isTracking) return;

    var lat = position.coords.latitude;
    var lng = position.coords.longitude;

    var location = {
        lat: lat,
        lng: lng,
        accuracy: position.coords.accuracy,
        timestamp: position.timestamp
    };

    // Add to pace calculator
    paceCalculator.addLocation(location);
    fullTrack.push({ lat: lat, lng: lng, timestamp: location.timestamp });

    // Get calculated pace
    var pace = paceCalculator.getCurrentPace();
    var targetPace = paceCalculator.getTargetPace();

    if (pace > 0) {
        sendPaceUpdate(pace, targetPace);
    }

    // Segment detection (only if Strava is connected)
    if (StravaAuth.isAuthenticated()) {
        checkSegments(lat, lng);
    }

    updateGhostRace(lat, lng);
}

/**
 * Check for segment entry/exit
 */
function checkSegments(lat, lng) {
    if (activeSegment === null) {
        // Not in a segment - check for segment start
        var segment = StravaSegments.findNearbySegmentStart(lat, lng, 50);
        if (segment) {
            console.log('Entered segment: ' + segment.name);
            activeSegment = segment;
            segmentStartTime = Date.now();

            // Get rival info and send alert
            var rivalName = segment.rival_name || 'Your PR';
            var rivalTime = segment.rival_time || segment.pr_time || 0;

            sendSegmentAlert(segment.name, rivalName, rivalTime);

            // Try to fetch fresh rival info
            StravaSegments.fetchSegmentRival(segment.id).then(function (rival) {
                if (rival) {
                    activeSegment.rival_name = rival.name;
                    activeSegment.rival_time = rival.time;
                    sendSegmentAlert(segment.name, rival.name, rival.time);
                }
            }).catch(function (err) {
                console.log('Could not fetch rival: ' + err);
            });
        }
    } else {
        // In a segment - check for segment end
        if (StravaSegments.isNearSegmentEnd(activeSegment, lat, lng, 50)) {
            var elapsed = Math.round((Date.now() - segmentStartTime) / 1000);
            console.log('Finished segment: ' + activeSegment.name + ' in ' + elapsed + 's');

            activeSegment = null;
            segmentStartTime = null;
            sendSegmentEnd();
        }
    }
}

/**
 * Handle GPS error
 */
function handleLocationError(error) {
    console.log('GPS Error: ' + error.message);
}

/**
 * Start GPS tracking
 */
function startDebugSimulator() {
    if (debugSimulatorTimer) {
        return;
    }

    console.log('Starting emulator debug simulator...');
    debugSimulatorTimer = setInterval(function () {
        if (!isTracking) {
            return;
        }

        var targetPace = paceCalculator.getTargetPace();
        var simulatedPace = targetPace + debugPaceOffset;
        var metersPerSecond = 1000 / simulatedPace;
        var deltaLat = (metersPerSecond / 111000);

        debugLat += deltaLat;
        debugLng += deltaLat * 0.2;
        debugPaceOffset = ((debugPaceOffset + 1) % 40) - 20;
        debugHr = 125 + (debugPaceOffset * 2);

        var location = {
            lat: debugLat,
            lng: debugLng,
            accuracy: 5,
            timestamp: Date.now()
        };

        paceCalculator.addLocation(location);
        fullTrack.push({ lat: debugLat, lng: debugLng, timestamp: location.timestamp });
        var pace = paceCalculator.getCurrentPace();
        if (pace > 0) {
            sendPaceUpdate(pace, targetPace);
        }
        sendHeartRate(debugHr);
        updateGhostRace(debugLat, debugLng);
    }, 1000);
}

function stopDebugSimulator() {
    if (debugSimulatorTimer) {
        clearInterval(debugSimulatorTimer);
        debugSimulatorTimer = null;
    }
}

function startTracking() {
    if (isTracking) return;

    console.log('Starting GPS tracking...');
    isTracking = true;
    isPaused = false;
    paceCalculator.reset();
    activeSegment = null;
    segmentStartTime = null;
    fullTrack = [];

    // Load cached segments
    StravaSegments.loadCachedSegments();

    // Race your most recent run as a ghost rival (offline, on-phone storage).
    startGhostRace();

    if (typeof navigator === 'undefined' || !navigator.geolocation) {
        startDebugSimulator();
        return;
    }

    var options = {
        enableHighAccuracy: true,
        maximumAge: 0,
        timeout: 5000
    };

    watchId = navigator.geolocation.watchPosition(
        handlePosition,
        handleLocationError,
        options
    );
}

/**
 * Stop GPS tracking
 */
function stopTracking() {
    if (!isTracking && !isPaused) return;

    console.log('Stopping GPS tracking...');
    isTracking = false;
    isPaused = false;

    stopDebugSimulator();

    if (watchId !== null) {
        navigator.geolocation.clearWatch(watchId);
        watchId = null;
    }

    if (activeSegment !== null) {
        sendSegmentEnd();
        activeSegment = null;
    }

    // Record the run to server
    var elapsed = paceCalculator.getElapsedTime();
    var distance = paceCalculator.getTotalDistance();
    var avgPace = paceCalculator.getAveragePace();
    var targetPace = paceCalculator.getTargetPace();

    // Save this run as a ghost rival for next time, then clear the watch rival.
    if (fullTrack.length >= 5) {
        GhostRacing.saveGhost(null, fullTrack, elapsed * 1000, distance);
    }
    if (GhostRacing.isRacing()) {
        GhostRacing.stopRacing();
    }
    sendSegmentEnd();

    // Calculate composure (how close to target pace)
    var composure = 100;
    if (avgPace > 0 && targetPace > 0) {
        var paceRatio = avgPace / targetPace;
        composure = Math.max(0, Math.min(100, 100 - Math.abs(1 - paceRatio) * 100));
    }

    // Save run if meaningful (> 30 seconds)
    if (elapsed > 30) {
        var runData = {
            distance: distance,
            elapsed: elapsed,
            avgPace: avgPace,
            composure: composure,
            escaped: composure >= 50,
            gpxData: paceCalculator.getGpsTrack()
        };

        ServerClient.saveRun(runData).then(function (result) {
            console.log('Run saved to server: ' + result.runId);
            // Optionally sync to Strava
            if (StravaAuth.isAuthenticated()) {
                ServerClient.syncToStrava(result.runId).then(function (stravaResult) {
                    console.log('Synced to Strava: ' + stravaResult.stravaUrl);
                }).catch(function (err) {
                    console.log('Strava sync failed: ' + err);
                });
            }
        }).catch(function (err) {
            console.log('Failed to save run: ' + err);
        });
    }
}

/**
 * Handle incoming message from watch
 */
function handleAppMessage(e) {
    var payload = e.payload;
    console.log('Received message from watch: ' + JSON.stringify(payload));

    WatchCommands.applyWatchPayload(payload, {
        startTracking: startTracking,
        stopTracking: stopTracking,
        pauseTracking: function () {
            isPaused = true;
            isTracking = false;
        },
        resumeTracking: function () {
            isPaused = false;
            isTracking = true;
        },
        setTargetPace: function (pace) {
            paceCalculator.setTargetPace(pace);
        }
    });
}

// Register PebbleKit JS event handlers
Pebble.addEventListener('ready', function () {
    console.log('Outrun PebbleKit JS ready!');

    // Initialize server client (register device if needed)
    ServerClient.init().then(function (status) {
        console.log('Server client initialized. Paired: ' + status.isPaired);
        if (!status.isPaired) {
            console.log('Pairing code: ' + status.pairingCode);
        }
        // Apply user settings if paired
        var settings = ServerClient.getUserSettings();
        if (settings && settings.targetPace) {
            paceCalculator.setTargetPace(settings.targetPace);
        }
    }).catch(function (err) {
        console.log('Server init failed, using offline mode: ' + err);
    });

    sendConnectedStatus();
    sendAllPlans().catch(function (err) {
        console.log('Failed to send plans: ' + err);
    });

    // Load cached segments on startup
    if (StravaAuth.isAuthenticated()) {
        console.log('Strava authenticated, loading segments...');
        StravaSegments.getSegments(false).then(function (segments) {
            console.log('Loaded ' + segments.length + ' segments');
        }).catch(function (err) {
            console.log('Failed to load segments: ' + err);
        });
    }
});

Pebble.addEventListener('appmessage', handleAppMessage);

// Configuration page for Strava OAuth
Pebble.addEventListener('showConfiguration', function () {
    var authUrl = StravaAuth.getAuthorizationUrl();
    console.log('Opening Strava auth: ' + authUrl);
    Pebble.openURL(authUrl);
});

Pebble.addEventListener('webviewclosed', function (e) {
    if (e && e.response) {
        try {
            var config = JSON.parse(decodeURIComponent(e.response));
            console.log('Config received: ' + JSON.stringify(config));

            // Handle Strava OAuth code
            if (config.code) {
                console.log('Exchanging Strava auth code...');
                StravaAuth.exchangeCodeForTokens(config.code).then(function (response) {
                    console.log('Strava auth successful!');
                    // Fetch segments after auth
                    StravaSegments.fetchStarredSegments().then(function (segments) {
                        console.log('Fetched ' + segments.length + ' segments');
                    });
                }).catch(function (err) {
                    console.log('Strava auth failed: ' + err);
                });
            }

            // Handle client credentials (if provided by config page)
            if (config.client_id && config.client_secret) {
                StravaAuth.setClientCredentials(config.client_id, config.client_secret);
            }

        } catch (err) {
            console.log('Error parsing config: ' + err);
        }
    }
});
