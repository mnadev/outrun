/**
 * Mock Prisma client for testing
 */

import { vi } from 'vitest'

export const mockDevice = {
  id: 'device-123',
  pairingCode: 'A7X3K2',
  name: 'Pebble',
  userId: null,
  user: null,
  lastSeen: new Date(),
  createdAt: new Date(),
}

export const mockUser = {
  id: 'user-123',
  name: 'Test User',
  email: 'test@example.com',
  premium: false,
  theme: 'classic',
  targetPace: 300,
  stravaId: null,
  stravaToken: null,
  stravaRefresh: null,
  stravaExpires: null,
  stripeCustomerId: null,
}

export const mockRun = {
  id: 'run-123',
  userId: 'user-123',
  distance: 5000,
  duration: 1800,
  avgPace: 360,
  composure: 85.0,
  escaped: true,
  closeCalls: 2,
  dangerTime: 120,
  gpxData: null,
  stravaId: null,
  theme: 'classic',
  createdAt: new Date(),
}

export const prismaMock = {
  device: {
    findUnique: vi.fn(),
    create: vi.fn(),
    update: vi.fn(),
  },
  user: {
    findUnique: vi.fn(),
    update: vi.fn(),
  },
  run: {
    findUnique: vi.fn(),
    create: vi.fn(),
    update: vi.fn(),
  },
}

vi.mock('@/lib/prisma', () => ({
  prisma: prismaMock,
}))
