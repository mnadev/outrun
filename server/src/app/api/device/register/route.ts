import { NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

// Generate a random 6-character pairing code
function generatePairingCode(): string {
  const chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // No confusing chars like O/0, I/1
  let code = "";
  for (let i = 0; i < 6; i++) {
    code += chars.charAt(Math.floor(Math.random() * chars.length));
  }
  return code;
}

/**
 * POST /api/device/register
 * Called by PebbleKit JS on first launch to get a pairing code
 */
export async function POST() {
  try {
    // Generate unique pairing code
    let pairingCode = generatePairingCode();
    let attempts = 0;

    // Ensure uniqueness
    while (attempts < 10) {
      const existing = await prisma.device.findUnique({
        where: { pairingCode },
      });
      if (!existing) break;
      pairingCode = generatePairingCode();
      attempts++;
    }

    // Create device record
    const device = await prisma.device.create({
      data: {
        pairingCode,
      },
    });

    return NextResponse.json({
      success: true,
      deviceId: device.id,
      pairingCode: device.pairingCode,
    });
  } catch (error) {
    console.error("Device registration error:", error);
    return NextResponse.json(
      { success: false, error: "Failed to register device" },
      { status: 500 }
    );
  }
}

/**
 * GET /api/device/register?deviceId=xxx
 * Check if device has been paired to a user
 */
export async function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const deviceId = searchParams.get("deviceId");

    if (!deviceId) {
      return NextResponse.json(
        { success: false, error: "Missing deviceId" },
        { status: 400 }
      );
    }

    const device = await prisma.device.findUnique({
      where: { id: deviceId },
      include: { user: true },
    });

    if (!device) {
      return NextResponse.json(
        { success: false, error: "Device not found" },
        { status: 404 }
      );
    }

    return NextResponse.json({
      success: true,
      paired: !!device.userId,
      pairingCode: device.pairingCode,
      user: device.user
        ? {
          id: device.user.id,
          name: device.user.name,
          premium: device.user.premium,
          theme: device.user.theme,
          targetPace: device.user.targetPace,
        }
        : null,
    });
  } catch (error) {
    console.error("Device check error:", error);
    return NextResponse.json(
      { success: false, error: "Failed to check device" },
      { status: 500 }
    );
  }
}
