/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SHUTDOWN_HANDLERS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SHUTDOWN_HANDLERS_H_

#include "base/functional/callback.h"

namespace brave_vpn::v2 {

// Installs platform-specific handlers that translate external termination
// requests (SIGINT/SIGTERM/SIGHUP on POSIX, console control events on Windows)
// into an invocation of a shutdown callback.
//
// Threading contract:
//   * Install() must be called on the main thread, after the main task runner
//   and the RunLoop quit closure have been set up, because the callback may
//   fire as soon as the handlers are installed.
//   * |shutdown_callback| may be invoked from an arbitrary background thread.
//   It is supposed to be cheap, thread-safe and idempotent: posts the real
//   shutdown work to the main thread.
//   * Following the Chromium convention, the first signal requests a graceful
//   shutdown; a second one falls through to the OS default disposition and
//   terminates the process immediately.
//
// The handlers are expected to stay installed for the lifetime of the process.
// The destructor restores the OS defaults but intentionally leaks the small
// amount of state shared with the (possibly still running) handler threads.
class ShutdownHandlers final {
 public:
  using ShutdownCallback = base::RepeatingClosure;

  explicit ShutdownHandlers(ShutdownCallback shutdown_callback);
  ~ShutdownHandlers();

  ShutdownHandlers(const ShutdownHandlers&) = delete;
  ShutdownHandlers& operator=(const ShutdownHandlers&) = delete;

  // Installs the platform-specific handlers. Must be called on the main thread,
  // and must not be called again after it succeeds. Returns true if the
  // handlers were installed, false if the platform does not support them (e.g.
  // no console on Windows).
  bool Install();

  // Must be called on the main thread after the run loop has quit and shutdown
  // work is complete. On Windows this releases a console control handler thread
  // that blocks on CTRL_CLOSE/LOGOFF/SHUTDOWN events to keep the OS from
  // terminating the process before cleanup finishes. No-op on POSIX.
  void SignalShutdownComplete();

 private:
  // Platform-specific teardown; called from the destructor if installed.
  void Uninstall();

  const ShutdownCallback shutdown_callback_;
  bool installed_ = false;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SHUTDOWN_HANDLERS_H_
