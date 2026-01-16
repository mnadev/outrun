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
var StravaAuth = require('./strava_auth');
var StravaSegments = require('./strava_segments');
var ServerClient = require('./server_client');

// State
var isTracking = false;
var watchId = null;
var paceCalculator = new PaceCalculator();
var activeSegment = null;
var segmentStartTime = null;

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
    CONNECTED: 8
};

// Commands from watch
var Commands = {
    START: 1,
    STOP: 2,
    PAUSE: 3,
    RESUME: 4
};

/**
 * Send pace data to the watch
 */
function sendPaceUpdate(currentPace, targetPace) {
    var message = {};
    message[Keys.CURRENT_PACE] = currentPace;
    message[Keys.TARGET_PACE] = targetPace;
    message[Keys.IS_RUNNING] = isTracking ? 1 : 0;

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
function startTracking() {
    if (isTracking) return;

    console.log('Starting GPS tracking...');
    isTracking = true;
    paceCalculator.reset();
    activeSegment = null;
    segmentStartTime = null;

    // Load cached segments
    StravaSegments.loadCachedSegments();

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
    if (!isTracking) return;

    console.log('Stopping GPS tracking...');
    isTracking = false;

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

    // Handle commands
    if (payload[Keys.COMMAND] !== undefined) {
        switch (payload[Keys.COMMAND]) {
            case Commands.START:
                startTracking();
                break;
            case Commands.STOP:
                stopTracking();
                break;
            case Commands.PAUSE:
                isTracking = false;
                break;
            case Commands.RESUME:
                isTracking = true;
                break;
        }
    }

    // Handle target pace update
    if (payload[Keys.TARGET_PACE] !== undefined) {
        paceCalculator.setTargetPace(payload[Keys.TARGET_PACE]);
    }
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
