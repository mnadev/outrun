/**
 * Tests for /api/run/save
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { prismaMock, mockDevice, mockUser, mockRun } from '../mocks/prisma'

describe('/api/run/save', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  describe('POST', () => {
    it('returns 400 for invalid run data', async () => {
      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ invalid: 'data' }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(response.status).toBe(400)
      expect(data.success).toBe(false)
    })

    it('returns 404 for unknown device', async () => {
      prismaMock.device.findUnique.mockResolvedValue(null)

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'unknown',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 85,
          escaped: true,
        }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(response.status).toBe(404)
      expect(data.error).toBe('Device not found')
    })

    it('returns 400 for unpaired device', async () => {
      prismaMock.device.findUnique.mockResolvedValue({ ...mockDevice, userId: null, user: null })

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'device-123',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 85,
          escaped: true,
        }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(response.status).toBe(400)
      expect(data.error).toBe('Device not paired')
    })

    it('saves run for paired device', async () => {
      const pairedDevice = { ...mockDevice, userId: 'user-123', user: mockUser }
      prismaMock.device.findUnique.mockResolvedValue(pairedDevice)
      prismaMock.device.update.mockResolvedValue(pairedDevice)
      prismaMock.run.create.mockResolvedValue(mockRun)

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'device-123',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 85,
          escaped: true,
        }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(response.status).toBe(200)
      expect(data.success).toBe(true)
      expect(data.runId).toBe('run-123')
    })

    it('returns escaped message for survival', async () => {
      const pairedDevice = { ...mockDevice, userId: 'user-123', user: mockUser }
      prismaMock.device.findUnique.mockResolvedValue(pairedDevice)
      prismaMock.device.update.mockResolvedValue(pairedDevice)
      prismaMock.run.create.mockResolvedValue({ ...mockRun, escaped: true })

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'device-123',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 85,
          escaped: true,
        }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(data.message).toBe('You ESCAPED!')
    })

    it('returns caught message for failure', async () => {
      const pairedDevice = { ...mockDevice, userId: 'user-123', user: mockUser }
      prismaMock.device.findUnique.mockResolvedValue(pairedDevice)
      prismaMock.device.update.mockResolvedValue(pairedDevice)
      prismaMock.run.create.mockResolvedValue({ ...mockRun, escaped: false })

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'device-123',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 30,
          escaped: false,
        }),
      })

      const response = await POST(request)
      const data = await response.json()

      expect(data.message).toBe('You were CAUGHT!')
    })

    it('updates device lastSeen', async () => {
      const pairedDevice = { ...mockDevice, userId: 'user-123', user: mockUser }
      prismaMock.device.findUnique.mockResolvedValue(pairedDevice)
      prismaMock.device.update.mockResolvedValue(pairedDevice)
      prismaMock.run.create.mockResolvedValue(mockRun)

      const { POST } = await import('@/app/api/run/save/route')

      const request = new Request('http://localhost/api/run/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          deviceId: 'device-123',
          distance: 5000,
          duration: 1800,
          avgPace: 360,
          composure: 85,
          escaped: true,
        }),
      })

      await POST(request)

      expect(prismaMock.device.update).toHaveBeenCalledWith({
        where: { id: 'device-123' },
        data: { lastSeen: expect.any(Date) },
      })
    })
  })
})
