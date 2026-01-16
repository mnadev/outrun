import { NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { z } from "zod";

// Request validation
const RunSchema = z.object({
  deviceId: z.string(),
  distance: z.number().min(0),
  duration: z.number().min(0),
  avgPace: z.number().min(0),
  composure: z.number().min(0).max(100),
  escaped: z.boolean(),
  closeCalls: z.number().min(0).optional(),
  dangerTime: z.number().min(0).optional(),
  gpxData: z.array(z.object({
    lat: z.number(),
    lng: z.number(),
    timestamp: z.number(),
  })).optional(),
  theme: z.string().optional(),
});

/**
 * POST /api/run/save
 * Save a completed run from PebbleKit JS
 */
export async function POST(request: Request) {
  try {
    const body = await request.json();
    const result = RunSchema.safeParse(body);

    if (!result.success) {
      return NextResponse.json(
        { success: false, error: "Invalid run data", details: result.error.issues },
        { status: 400 }
      );
    }

    const data = result.data;

    // Find device and verify it's paired
    const device = await prisma.device.findUnique({
      where: { id: data.deviceId },
      include: { user: true },
    });

    if (!device) {
      return NextResponse.json(
        { success: false, error: "Device not found" },
        { status: 404 }
      );
    }

    if (!device.userId) {
      return NextResponse.json(
        { success: false, error: "Device not paired" },
        { status: 400 }
      );
    }

    // Update device last seen
    await prisma.device.update({
      where: { id: device.id },
      data: { lastSeen: new Date() },
    });

    // Create run record
    const run = await prisma.run.create({
      data: {
        userId: device.userId,
        distance: data.distance,
        duration: data.duration,
        avgPace: data.avgPace,
        composure: data.composure,
        escaped: data.escaped,
        closeCalls: data.closeCalls || 0,
        dangerTime: data.dangerTime || 0,
        gpxData: data.gpxData || null,
        theme: data.theme || "classic",
      },
    });

    return NextResponse.json({
      success: true,
      runId: run.id,
      message: data.escaped ? "You ESCAPED!" : "You were CAUGHT!",
    });
  } catch (error) {
    console.error("Run save error:", error);
    return NextResponse.json(
      { success: false, error: "Failed to save run" },
      { status: 500 }
    );
  }
}
