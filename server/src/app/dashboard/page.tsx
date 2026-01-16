import { getServerSession } from "next-auth";
import { redirect } from "next/navigation";
import Link from "next/link";
import { prisma } from "@/lib/prisma";
import { authOptions } from "@/lib/auth";

export default async function DashboardPage() {
  const session = await getServerSession(authOptions);

  if (!session?.user?.email) {
    redirect("/login");
  }

  const user = await prisma.user.findUnique({
    where: { email: session.user.email },
    include: {
      runs: {
        orderBy: { createdAt: "desc" },
        take: 10,
      },
      devices: true,
    },
  });

  if (!user) {
    redirect("/login");
  }

  // Calculate stats
  const totalRuns = user.runs.length;
  const escapedCount = user.runs.filter((r) => r.escaped).length;
  const survivalRate = totalRuns > 0 ? Math.round((escapedCount / totalRuns) * 100) : 100;
  const avgComposure = totalRuns > 0
    ? Math.round(user.runs.reduce((sum, r) => sum + r.composure, 0) / totalRuns)
    : 0;

  // Get horror rank
  const fearFactor = (survivalRate * 0.5) + (avgComposure * 0.5);
  let rank = { name: "Victim", emoji: "💀" };
  if (fearFactor >= 95) rank = { name: "Final Girl", emoji: "👑" };
  else if (fearFactor >= 85) rank = { name: "Survivor", emoji: "🏆" };
  else if (fearFactor >= 75) rank = { name: "Fighter", emoji: "💪" };
  else if (fearFactor >= 60) rank = { name: "Runner", emoji: "🏃" };
  else if (fearFactor >= 40) rank = { name: "Nervous", emoji: "😰" };
  else if (fearFactor >= 20) rank = { name: "Stumbler", emoji: "😱" };

  return (
    <main className="min-h-screen p-8">
      <div className="max-w-4xl mx-auto">
        {/* Header */}
        <div className="flex justify-between items-center mb-8">
          <div>
            <h1 className="text-3xl font-bold horror-text">Dashboard</h1>
            <p className="text-gray-400">Welcome back, {user.name || "Runner"}</p>
          </div>
          <div className="flex gap-4">
            <Link href="/pair" className="text-gray-400 hover:text-white">
              Pair Device
            </Link>
            <Link href="/settings" className="text-gray-400 hover:text-white">
              Settings
            </Link>
          </div>
        </div>

        {/* Stats Grid */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-4 mb-8">
          <div className="horror-card p-4 text-center">
            <div className="text-4xl mb-2">{rank.emoji}</div>
            <div className="text-xl font-bold">{rank.name}</div>
            <div className="text-gray-500 text-sm">Horror Rank</div>
          </div>

          <div className="horror-card p-4 text-center">
            <div className="text-4xl font-bold text-red-500">{survivalRate}%</div>
            <div className="text-gray-500 text-sm">Survival Rate</div>
          </div>

          <div className="horror-card p-4 text-center">
            <div className="text-4xl font-bold text-yellow-500">{avgComposure}%</div>
            <div className="text-gray-500 text-sm">Avg Composure</div>
          </div>

          <div className="horror-card p-4 text-center">
            <div className="text-4xl font-bold">{totalRuns}</div>
            <div className="text-gray-500 text-sm">Total Runs</div>
          </div>
        </div>

        {/* Premium Badge */}
        {user.premium && (
          <div className="horror-card p-4 mb-8 border-yellow-600">
            <span className="text-yellow-500">⭐ Premium</span>
            <span className="text-gray-400 ml-2">All themes unlocked</span>
          </div>
        )}

        {/* Devices */}
        <div className="horror-card p-6 mb-8">
          <h2 className="text-xl font-bold mb-4">Paired Devices</h2>
          {user.devices.length === 0 ? (
            <div className="text-gray-500">
              No devices paired.{" "}
              <Link href="/pair" className="text-red-500 hover:text-red-400">
                Pair your watch →
              </Link>
            </div>
          ) : (
            <ul className="space-y-2">
              {user.devices.map((device) => (
                <li key={device.id} className="flex justify-between items-center">
                  <span>⌚ {device.name}</span>
                  <span className="text-gray-500 text-sm">
                    Last seen: {new Date(device.lastSeen).toLocaleDateString()}
                  </span>
                </li>
              ))}
            </ul>
          )}
        </div>

        {/* Recent Runs */}
        <div className="horror-card p-6">
          <h2 className="text-xl font-bold mb-4">Recent Runs</h2>
          {user.runs.length === 0 ? (
            <div className="text-gray-500">No runs yet. Start running!</div>
          ) : (
            <div className="space-y-4">
              {user.runs.map((run) => (
                <div
                  key={run.id}
                  className={`flex justify-between items-center p-4 rounded-lg ${run.escaped ? "bg-green-900/20" : "bg-red-900/20"
                    }`}
                >
                  <div>
                    <span className="text-2xl mr-2">
                      {run.escaped ? "🏃" : "💀"}
                    </span>
                    <span className="font-semibold">
                      {(run.distance / 1000).toFixed(2)} km
                    </span>
                    <span className="text-gray-500 ml-2">
                      {Math.floor(run.duration / 60)}:{(run.duration % 60).toString().padStart(2, "0")}
                    </span>
                  </div>
                  <div className="text-right">
                    <div className="text-sm">
                      Composure: <span className="text-yellow-500">{run.composure.toFixed(0)}%</span>
                    </div>
                    <div className="text-gray-500 text-xs">
                      {new Date(run.createdAt).toLocaleDateString()}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </main>
  );
}
