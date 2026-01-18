/**
 * Jest test setup - runs before each test file
 */

// Mock localStorage for all tests
const localStorageMock = {
  store: {},
  getItem: function (key) { return this.store[key] || null; },
  setItem: function (key, value) { this.store[key] = value; },
  removeItem: function (key) { delete this.store[key]; },
  clear: function () { this.store = {}; }
};

global.localStorage = localStorageMock;

// Mock console.log to reduce noise (optional)
// global.console.log = jest.fn();

// Reset localStorage before each test
beforeEach(() => {
  localStorageMock.clear();
});

// Handle unhandled promise rejections
process.on('unhandledRejection', () => {
  // Silently handle - tests should catch rejections
});
