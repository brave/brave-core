/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"

#include <windows.h>

#include <atomic>
#include <utility>

#include "base/check.h"
#include "base/debug/leak_annotations.h"
#include "base/functional/callback.h"
#include "base/synchronization/waitable_event.h"
#include "base/time/time.h"

namespace brave_vpn::v2 {
namespace {
// State shared with the console control handler, which the OS runs on a thread
// it injects into this process. Heap-allocated and intentionally leaked so that
// an in-flight handler can never observe destroyed objects.

// Set once in Install() before SetConsoleCtrlHandler(), so handler threads
// always see the fully-published pointer. Never rewritten afterward, so a
// non-atomic pointer is race-free. Pointee is run at most once, guaranteed by
// g_shutdown_requested.
base::OnceClosure* g_shutdown_callback = nullptr;

// Same as g_shutdown_callback: written once before SetConsoleCtrlHandler(),
// never after, so the plain pointer needs no atomic. Pointee is a
// WaitableEvent, which is internally thread-safe, so the main-thread Signal()
// vs. handler-thread Wait() handshake is not a data race.
base::WaitableEvent* g_shutdown_complete_event = nullptr;

// The first event requests a graceful shutdown; subsequent events fall through
// to the default handler, which terminates the process. This mirrors the POSIX
// "second signal kills immediately" convention.
// Sequential consistency (the default) is intentional: while a weaker ordering
// would be correct today, one might later start relying on that flag to publish
// other state, and it'll turn into a silent race. The stronger ordering costs
// nothing on this cold path.
std::atomic<bool> g_shutdown_requested{false};

// How long the handler may hold off process termination on
// CTRL_CLOSE/LOGOFF/SHUTDOWN events. The OS grants roughly five seconds before
// terminating the process regardless, so stay under that.
constexpr base::TimeDelta kShutdownCompleteTimeout = base::Seconds(4);

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type) {
  switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      if (g_shutdown_requested.exchange(true)) {
        // Second Ctrl+C: let the default handler terminate the process, in case
        // the graceful shutdown has hung.
        return FALSE;
      }
      std::move(*g_shutdown_callback).Run();
      // TRUE suppresses the default handler, which would terminate the process
      // immediately and skip our cleanup.
      return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      // For these events Windows terminates the process as soon as this handler
      // returns, regardless of the return value. Request shutdown, then block
      // here until the main thread reports completion (or the OS deadline
      // approaches), so the graceful path has time to run.
      if (!g_shutdown_requested.exchange(true)) {
        std::move(*g_shutdown_callback).Run();
      }
      g_shutdown_complete_event->TimedWait(kShutdownCompleteTimeout);
      return TRUE;
    default:
      return FALSE;
  }
}

}  // namespace

bool ShutdownHandlers::Install() {
  CHECK(!installed_);
  if (!::GetConsoleWindow()) {
    // If the process has no console, the OS will not send any console events.
    return false;
  }
  installed_ = true;

  // Set up the shared state before registering the handler so the handler can
  // never observe null globals.
  g_shutdown_callback = new base::OnceClosure(std::move(shutdown_callback_));
  g_shutdown_complete_event =
      new base::WaitableEvent(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  ANNOTATE_LEAKING_OBJECT_PTR(g_shutdown_callback);
  ANNOTATE_LEAKING_OBJECT_PTR(g_shutdown_complete_event);
  CHECK(::SetConsoleCtrlHandler(&ConsoleCtrlHandler, /*Add=*/TRUE));
  return true;
}

void ShutdownHandlers::SignalShutdownComplete() {
  if (g_shutdown_complete_event) {
    g_shutdown_complete_event->Signal();
  }
}

void ShutdownHandlers::Uninstall() {
  ::SetConsoleCtrlHandler(&ConsoleCtrlHandler, /*Add=*/FALSE);
  // `g_shutdown_callback` and `g_shutdown_complete_event` are intentionally
  // leaked: a handler dispatched just before removal may still be using
  // them.
}

}  // namespace brave_vpn::v2
