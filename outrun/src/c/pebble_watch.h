/**
 * pebble_watch.h - Installs the Pebble implementation of WatchInterface.
 */

#pragma once

/**
 * Register the Pebble platform interface as the active watch.
 * Call once during app init, before any run timing or feedback.
 */
void pebble_watch_install(void);
