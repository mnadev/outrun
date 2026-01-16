import { NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

const STRAVA_TOKEN_URL = "https://www.strava.com/oauth/token";
const STRAVA_API_BASE = "https://www.strava.com/api/v3";

/**
 * POST /api/strava/sync
 * Upload a run to Strava as a new activity
 */
export async function POST(request: Request) {
  try {
    const { runId, deviceId } = await request.json();

    if (!runId || !deviceId) {
      return NextResponse.json(
        { success: false, error: "Missing runId or deviceId" },
        { status: 400 }
      );
    }

    // Get device and user
    const device = await prisma.device.findUnique({
      where: { id: deviceId },
      include: { user: true },
    });

    if (!device?.user) {
      return NextResponse.json(
        { success: false, error: "Device not paired" },
        { status: 400 }
      );
    }

    const user = device.user;

    if (!user.stravaToken) {
      return NextResponse.json(
        { success: false, error: "Strava not connected" },
        { status: 400 }
      );
    }

    // Get run
    const run = await prisma.run.findUnique({
      where: { id: runId },
    });

    if (!run) {
      return NextResponse.json(
        { success: false, error: "Run not found" },
        { status: 404 }
      );
    }

    if (run.stravaId) {
      return NextResponse.json(
        { success: false, error: "Already synced to Strava" },
        { status: 400 }
      );
    }

    // Refresh token if expired
    let accessToken = user.stravaToken;
    if (user.stravaExpires && user.stravaExpires < new Date()) {
      accessToken = await refreshStravaToken(user.id, user.stravaRefresh!);
    }

    // Create Strava activity
    const startDate = new Date(run.createdAt);
    startDate.setSeconds(startDate.getSeconds() - run.duration);

    const activityData = {
      name: run.escaped
        ? `🏃 Outrun: ESCAPED! (${run.theme})`
        : `💀 Outrun: CAUGHT! (${run.theme})`,
      type: "Run",
      start_date_local: startDate.toISOString(),
      elapsed_time: run.duration,
      distance: run.distance,
      description: `Outrun run with ${run.composure.toFixed(0)}% composure. ${run.closeCalls} close calls.`,
    };

    const stravaRes = await fetch(`${STRAVA_API_BASE}/activities`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${accessToken}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify(activityData),
    });

    if (!stravaRes.ok) {
      const error = await stravaRes.text();
      console.error("Strava API error:", error);
      return NextResponse.json(
        { success: false, error: "Strava upload failed" },
        { status: 500 }
      );
    }

    const stravaActivity = await stravaRes.json();

    // Update run with Strava ID
    await prisma.run.update({
      where: { id: runId },
      data: { stravaId: stravaActivity.id.toString() },
    });

    return NextResponse.json({
      success: true,
      stravaActivityId: stravaActivity.id,
      stravaUrl: `https://www.strava.com/activities/${stravaActivity.id}`,
    });
  } catch (error) {
    console.error("Strava sync error:", error);
    return NextResponse.json(
      { success: false, error: "Failed to sync to Strava" },
      { status: 500 }
    );
  }
}

async function refreshStravaToken(userId: string, refreshToken: string): Promise<string> {
  const response = await fetch(STRAVA_TOKEN_URL, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams({
      client_id: process.env.STRAVA_CLIENT_ID!,
      client_secret: process.env.STRAVA_CLIENT_SECRET!,
      grant_type: "refresh_token",
      refresh_token: refreshToken,
    }),
  });

  const data = await response.json();

  // Update user tokens
  await prisma.user.update({
    where: { id: userId },
    data: {
      stravaToken: data.access_token,
      stravaRefresh: data.refresh_token,
      stravaExpires: new Date(data.expires_at * 1000),
    },
  });

  return data.access_token;
}
