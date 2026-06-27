/**
 * persist_stub.h - In-memory Pebble persistent-storage stub for host tests.
 *
 * The persist_* functions themselves are declared in the stub <pebble.h>; this
 * header adds test-only helpers to reset the store and inject raw bytes (to
 * simulate legacy/foreign persisted data).
 */

#ifndef PERSIST_STUB_H
#define PERSIST_STUB_H

#include <stddef.h>
#include <stdint.h>

/** Clear the in-memory persistent store (call between tests). */
void persist_stub_reset(void);

/** Inject raw bytes for a key, simulating legacy or foreign stored data. */
void persist_stub_write_raw(uint32_t key, const void *data, size_t length);

#endif // PERSIST_STUB_H
