"use client";

import { signIn } from "next-auth/react";
import { useState } from "react";
import Link from "next/link";

export default function LoginPage() {
  const [email, setEmail] = useState("");
  const [status, setStatus] = useState<"idle" | "loading" | "success" | "error">("idle");

  const handleEmailLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setStatus("loading");

    try {
      await signIn("email", { email, callbackUrl: "/dashboard" });
      setStatus("success");
    } catch {
      setStatus("error");
    }
  };

  const handleStravaLogin = () => {
    signIn("strava", { callbackUrl: "/dashboard" });
  };

  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-8">
      <div className="max-w-md w-full">
        <Link href="/" className="text-gray-500 hover:text-white mb-8 block">
          ← Back
        </Link>

        <h1 className="text-4xl font-bold mb-2 horror-text">Sign In</h1>
        <p className="text-gray-400 mb-8">
          Track your runs and sync with Strava
        </p>

        {/* Strava Login */}
        <button
          onClick={handleStravaLogin}
          className="w-full bg-orange-600 hover:bg-orange-700 text-white px-8 py-4 rounded-lg font-semibold text-lg transition-colors flex items-center justify-center gap-2 mb-6"
        >
          <span>🏃</span> Continue with Strava
        </button>

        <div className="relative my-6">
          <div className="absolute inset-0 flex items-center">
            <div className="w-full border-t border-gray-700"></div>
          </div>
          <div className="relative flex justify-center text-sm">
            <span className="px-2 bg-black text-gray-500">or</span>
          </div>
        </div>

        {/* Email Login */}
        <form onSubmit={handleEmailLogin} className="space-y-4">
          <div>
            <input
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="you@example.com"
              className="w-full bg-gray-900 border border-gray-700 rounded-lg px-4 py-3 focus:border-red-500 focus:outline-none"
              required
            />
          </div>

          <button
            type="submit"
            disabled={status === "loading"}
            className="w-full bg-gray-800 hover:bg-gray-700 disabled:bg-gray-900 disabled:cursor-not-allowed text-white px-8 py-3 rounded-lg font-semibold transition-colors"
          >
            {status === "loading" ? "Sending..." : "Sign in with Email"}
          </button>
        </form>

        {status === "success" && (
          <div className="mt-6 p-4 bg-green-900/50 border border-green-700 rounded-lg">
            <p className="text-green-400">✓ Check your email for a magic link</p>
          </div>
        )}

        {status === "error" && (
          <div className="mt-6 p-4 bg-red-900/50 border border-red-700 rounded-lg">
            <p className="text-red-400">✗ Failed to send email. Try again.</p>
          </div>
        )}
      </div>
    </main>
  );
}
