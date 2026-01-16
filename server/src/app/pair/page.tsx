"use client";

import { useState } from "react";
import Link from "next/link";

export default function PairPage() {
  const [code, setCode] = useState("");
  const [status, setStatus] = useState<"idle" | "loading" | "success" | "error">("idle");
  const [message, setMessage] = useState("");

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();

    if (code.length !== 6) {
      setStatus("error");
      setMessage("Please enter a 6-character code");
      return;
    }

    setStatus("loading");

    try {
      const response = await fetch("/api/device/pair", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ pairingCode: code.toUpperCase() }),
      });

      const data = await response.json();

      if (data.success) {
        setStatus("success");
        setMessage("Device paired successfully!");
      } else {
        setStatus("error");
        setMessage(data.error || "Failed to pair device");
      }
    } catch {
      setStatus("error");
      setMessage("Network error. Please try again.");
    }
  };

  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-8">
      <div className="max-w-md w-full">
        <Link href="/" className="text-gray-500 hover:text-white mb-8 block">
          ← Back
        </Link>

        <h1 className="text-4xl font-bold mb-2 horror-text">Pair Your Watch</h1>
        <p className="text-gray-400 mb-8">
          Enter the 6-character code shown on your Pebble
        </p>

        <form onSubmit={handleSubmit} className="space-y-6">
          <div>
            <input
              type="text"
              value={code}
              onChange={(e) => setCode(e.target.value.toUpperCase().replace(/[^A-Z0-9]/g, "").slice(0, 6))}
              placeholder="A7X3K2"
              className="w-full text-4xl text-center font-mono tracking-[0.5em] bg-gray-900 border border-gray-700 rounded-lg px-4 py-6 focus:border-red-500 focus:outline-none"
              maxLength={6}
              autoFocus
            />
          </div>

          <button
            type="submit"
            disabled={status === "loading" || code.length !== 6}
            className="w-full bg-red-600 hover:bg-red-700 disabled:bg-gray-700 disabled:cursor-not-allowed text-white px-8 py-4 rounded-lg font-semibold text-lg transition-colors"
          >
            {status === "loading" ? "Pairing..." : "Pair Device"}
          </button>
        </form>

        {status === "success" && (
          <div className="mt-6 p-4 bg-green-900/50 border border-green-700 rounded-lg">
            <p className="text-green-400">✓ {message}</p>
            <Link href="/dashboard" className="text-green-300 hover:text-white mt-2 block">
              Go to Dashboard →
            </Link>
          </div>
        )}

        {status === "error" && (
          <div className="mt-6 p-4 bg-red-900/50 border border-red-700 rounded-lg">
            <p className="text-red-400">✗ {message}</p>
          </div>
        )}

        <div className="mt-12 text-gray-500 text-sm">
          <h3 className="font-semibold mb-2">How to find your code:</h3>
          <ol className="list-decimal list-inside space-y-1">
            <li>Open Outrun on your Pebble</li>
            <li>Press and hold SELECT</li>
            <li>The pairing code will appear</li>
          </ol>
        </div>
      </div>
    </main>
  );
}
