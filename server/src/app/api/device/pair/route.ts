import { NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getServerSession } from "next-auth";

/**
 * POST /api/device/pair
 * User enters pairing code on web to link device to their account
 */
export async function POST(request: Request) {
  try {
    const session = await getServerSession();

    if (!session?.user?.email) {
      return NextResponse.json(
        { success: false, error: "Not authenticated" },
        { status: 401 }
      );
    }

    const { pairingCode } = await request.json();

    if (!pairingCode || typeof pairingCode !== "string") {
      return NextResponse.json(
        { success: false, error: "Missing pairing code" },
        { status: 400 }
      );
    }

    // Find device by pairing code
    const device = await prisma.device.findUnique({
      where: { pairingCode: pairingCode.toUpperCase() },
    });

    if (!device) {
      return NextResponse.json(
        { success: false, error: "Invalid pairing code" },
        { status: 404 }
      );
    }

    if (device.userId) {
      return NextResponse.json(
        { success: false, error: "Device already paired" },
        { status: 400 }
      );
    }

    // Find user
    const user = await prisma.user.findUnique({
      where: { email: session.user.email },
    });

    if (!user) {
      return NextResponse.json(
        { success: false, error: "User not found" },
        { status: 404 }
      );
    }

    // Link device to user
    await prisma.device.update({
      where: { id: device.id },
      data: { userId: user.id },
    });

    return NextResponse.json({
      success: true,
      message: "Device paired successfully",
      device: {
        id: device.id,
        name: device.name,
      },
    });
  } catch (error) {
    console.error("Pairing error:", error);
    return NextResponse.json(
      { success: false, error: "Failed to pair device" },
      { status: 500 }
    );
  }
}
