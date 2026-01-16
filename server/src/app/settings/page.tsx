"use client";

import { useSession, signOut } from "next-auth/react";
import { useState, useEffect } from "react";
import Link from "next/link";
import { redirect } from "next/navigation";

export default function SettingsPage() {
  const { data: session, status } = useSession();
  const [theme, setTheme] = useState("classic");
  const [targetPace, setTargetPace] = useState(300);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    if (status === "unauthenticated") {
      redirect("/login");
    }
  }, [status]);

  const themes = [
    { id: "classic", name: "Classic Slasher", emoji: "🔪" },
    { id: "paranormal", name: "Paranormal", emoji: "👻" },
    { id: "zombie", name: "Zombie Horde", emoji: "🧟" },
    { id: "alien", name: "Alien Pursuit", emoji: "👽" },
    { id: "werewolf", name: "Full Moon", emoji: "🐺" },
  ];

  const handleSave = async () => {
    setSaving(true);
    try {
      await fetch("/api/user/settings", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ theme, targetPace }),
      });
    } catch (error) {
      console.error("Failed to save settings:", error);
    }
    setSaving(false);
  };

  const formatPace = (seconds: number) => {
    const min = Math.floor(seconds / 60);
    const sec = seconds % 60;
    return `${min}:${sec.toString().padStart(2, "0")}/km`;
  };

  if (status === "loading") {
    return <div className="min-h-screen flex items-center justify-center">Loading...</div>;
  }

  return (
    <main className="min-h-screen p-8">
      <div className="max-w-2xl mx-auto">
        <Link href="/dashboard" className="text-gray-500 hover:text-white mb-8 block">
          ← Back to Dashboard
        </Link>

        <h1 className="text-3xl font-bold mb-8 horror-text">Settings</h1>

        {/* Account */}
        <div className="horror-card p-6 mb-6">
          <h2 className="text-xl font-bold mb-4">Account</h2>
          <p className="text-gray-400 mb-4">{session?.user?.email || "Not signed in"}</p>
          <button
            onClick={() => signOut({ callbackUrl: "/" })}
            className="text-red-500 hover:text-red-400"
          >
            Sign Out
          </button>
        </div>

        {/* Theme Selection */}
        <div className="horror-card p-6 mb-6">
          <h2 className="text-xl font-bold mb-4">Stalker Theme</h2>
          <div className="grid grid-cols-1 gap-2">
            {themes.map((t) => (
              <button
                key={t.id}
                onClick={() => setTheme(t.id)}
                className={`p-4 rounded-lg text-left flex items-center gap-3 transition-colors ${theme === t.id
                    ? "bg-red-900/50 border border-red-600"
                    : "bg-gray-900 hover:bg-gray-800"
                  }`}
              >
                <span className="text-2xl">{t.emoji}</span>
                <span className="font-semibold">{t.name}</span>
              </button>
            ))}
          </div>
        </div>

        {/* Target Pace */}
        <div className="horror-card p-6 mb-6">
          <h2 className="text-xl font-bold mb-4">Target Pace</h2>
          <p className="text-gray-400 mb-4">
            Current: <span className="text-white font-mono">{formatPace(targetPace)}</span>
          </p>
          <input
            type="range"
            min={180}
            max={600}
            step={15}
            value={targetPace}
            onChange={(e) => setTargetPace(parseInt(e.target.value))}
            className="w-full"
          />
          <div className="flex justify-between text-gray-500 text-sm mt-2">
            <span>3:00/km</span>
            <span>10:00/km</span>
          </div>
        </div>

        {/* Save Button */}
        <button
          onClick={handleSave}
          disabled={saving}
          className="w-full bg-red-600 hover:bg-red-700 disabled:bg-gray-700 text-white px-8 py-4 rounded-lg font-semibold text-lg transition-colors"
        >
          {saving ? "Saving..." : "Save Settings"}
        </button>
      </div>
    </main>
  );
}
