import Link from "next/link";

export default function Home() {
  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-8">
      {/* Hero */}
      <div className="text-center max-w-2xl">
        <h1 className="text-6xl font-bold mb-4 horror-text">
          🏃 OUTRUN
        </h1>
        <p className="text-2xl text-gray-400 mb-2">
          The Social Horror Pacer
        </p>
        <p className="text-lg text-gray-500 mb-8">
          Run like your life depends on it... because in Outrun, it does.
        </p>

        {/* CTA Buttons */}
        <div className="flex gap-4 justify-center mb-12">
          <Link
            href="/pair"
            className="bg-red-600 hover:bg-red-700 text-white px-8 py-3 rounded-lg font-semibold transition-colors"
          >
            Pair Your Watch
          </Link>
          <Link
            href="/dashboard"
            className="bg-gray-800 hover:bg-gray-700 text-white px-8 py-3 rounded-lg font-semibold transition-colors"
          >
            View Dashboard
          </Link>
        </div>
      </div>

      {/* Features */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 max-w-4xl">
        <div className="horror-card p-6">
          <div className="text-3xl mb-3">🔪</div>
          <h3 className="text-xl font-semibold mb-2">The Killer</h3>
          <p className="text-gray-400">
            Fall behind pace and IT gets closer. Feel the heartbeat intensify.
          </p>
        </div>

        <div className="horror-card p-6">
          <div className="text-3xl mb-3">👻</div>
          <h3 className="text-xl font-semibold mb-2">Ghost Racing</h3>
          <p className="text-gray-400">
            Race against your past runs. Beat your ghost or become one.
          </p>
        </div>

        <div className="horror-card p-6">
          <div className="text-3xl mb-3">🏆</div>
          <h3 className="text-xl font-semibold mb-2">Survive Together</h3>
          <p className="text-gray-400">
            Join leagues, compare Composure scores, climb the leaderboard.
          </p>
        </div>
      </div>

      {/* Horror Ranks */}
      <div className="mt-12 text-center">
        <p className="text-gray-500 mb-2">Horror Ranks</p>
        <p className="text-lg">
          👑 Final Girl → 🏆 Survivor → 💪 Fighter → 🏃 Runner → 😰 Nervous → 😱 Stumbler → 💀 Victim
        </p>
      </div>
    </main>
  );
}
