/**
 * persist_stub.c - In-memory implementation of the Pebble persist_* API.
 *
 * Backs settings persistence tests with a simple key->blob store shared across
 * translation units, so a test can write legacy bytes and verify how
 * settings_init() reacts.
 */

#include "persist_stub.h"

#include <pebble.h>
#include <string.h>

#define PERSIST_STUB_MAX_KEYS 16
#define PERSIST_STUB_MAX_BLOB 256

typedef struct {
  bool used;
  uint32_t key;
  size_t length;
  uint8_t data[PERSIST_STUB_MAX_BLOB];
} PersistStubSlot;

static PersistStubSlot s_slots[PERSIST_STUB_MAX_KEYS];

static PersistStubSlot *stub_find(uint32_t key) {
  for (int i = 0; i < PERSIST_STUB_MAX_KEYS; i++) {
    if (s_slots[i].used && s_slots[i].key == key) {
      return &s_slots[i];
    }
  }
  return NULL;
}

static PersistStubSlot *stub_alloc(uint32_t key) {
  PersistStubSlot *slot = stub_find(key);
  if (slot) {
    return slot;
  }
  for (int i = 0; i < PERSIST_STUB_MAX_KEYS; i++) {
    if (!s_slots[i].used) {
      s_slots[i].used = true;
      s_slots[i].key = key;
      s_slots[i].length = 0;
      return &s_slots[i];
    }
  }
  return NULL;
}

void persist_stub_reset(void) { memset(s_slots, 0, sizeof(s_slots)); }

void persist_stub_write_raw(uint32_t key, const void *data, size_t length) {
  persist_write_data(key, data, length);
}

bool persist_exists(uint32_t key) { return stub_find(key) != NULL; }

int persist_read_data(uint32_t key, void *buffer, size_t length) {
  PersistStubSlot *slot = stub_find(key);
  if (!slot) {
    return 0;
  }
  size_t n = slot->length < length ? slot->length : length;
  memcpy(buffer, slot->data, n);
  return (int)n;
}

int persist_write_data(uint32_t key, const void *data, size_t length) {
  PersistStubSlot *slot = stub_alloc(key);
  if (!slot) {
    return 0;
  }
  size_t n = length < PERSIST_STUB_MAX_BLOB ? length : PERSIST_STUB_MAX_BLOB;
  memcpy(slot->data, data, n);
  slot->length = n;
  return (int)n;
}
