/**
 * Tests for /api/device/register
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { prismaMock, mockDevice } from '../mocks/prisma'

// Import after mocks are set up
import { POST, GET } from '@/app/api/device/register/route'

describe('/api/device/register', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  describe('POST', () => {
    it('creates a new device with pairing code', async () => {
      prismaMock.device.findUnique.mockResolvedValue(null) // No existing code
      prismaMock.device.create.mockResolvedValue(mockDevice)

      const response = await POST()
      const data = await response.json()

      expect(response.status).toBe(200)
      expect(data.success).toBe(true)
      expect(data.deviceId).toBe('device-123')
      expect(data.pairingCode).toBe('A7X3K2')
    })

    it('generates unique pairing code on collision', async () => {
      const existingDevice = { ...mockDevice, id: 'existing' }
      prismaMock.device.findUnique
        .mockResolvedValueOnce(existingDevice) // First code exists
        .mockResolvedValueOnce(null) // Second code is unique
      prismaMock.device.create.mockResolvedValue(mockDevice)

      const response = await POST()
      const data = await response.json()

      expect(response.status).toBe(200)
      expect(data.success).toBe(true)
      expect(prismaMock.device.findUnique).toHaveBeenCalledTimes(2)
    })
  })

  describe('GET', () => {
    it('returns 400 without deviceId', async () => {
      const request = new Request('http://localhost/api/device/register')
      const response = await GET(request)
      const data = await response.json()

      expect(response.status).toBe(400)
      expect(data.success).toBe(false)
      expect(data.error).toBe('Missing deviceId')
    })

    it('returns 404 for unknown device', async () => {
      prismaMock.device.findUnique.mockResolvedValue(null)

      const request = new Request('http://localhost/api/device/register?deviceId=unknown')
      const response = await GET(request)
      const data = await response.json()

      expect(response.status).toBe(404)
      expect(data.success).toBe(false)
    })

    it('returns paired status for known device', async () => {
      const pairedDevice = { ...mockDevice, userId: 'user-123', user: { id: 'user-123', name: 'Test', premium: false, theme: 'classic', targetPace: 300 } }
      prismaMock.device.findUnique.mockResolvedValue(pairedDevice)

      const request = new Request('http://localhost/api/device/register?deviceId=device-123')
      const response = await GET(request)
      const data = await response.json()

      expect(response.status).toBe(200)
      expect(data.success).toBe(true)
      expect(data.paired).toBe(true)
      expect(data.user.name).toBe('Test')
    })

    it('returns unpaired status for orphan device', async () => {
      prismaMock.device.findUnique.mockResolvedValue(mockDevice)

      const request = new Request('http://localhost/api/device/register?deviceId=device-123')
      const response = await GET(request)
      const data = await response.json()

      expect(response.status).toBe(200)
      expect(data.paired).toBe(false)
      expect(data.user).toBeNull()
    })
  })
})
