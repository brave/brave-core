/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Modeled on chrome/browser/shutdown_signal_handlers_posix.cc: the signal
// handler only performs async-signal-safe work (it writes one byte to a
// self-pipe); a dedicated detector thread blocks on the read end and invokes
// the shutdown callback from a normal thread context.

#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"

#include <signal.h>
#include <unistd.h>

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/debug/leak_annotations.h"
#include "base/functional/callback.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/platform_thread.h"

namespace brave_vpn::v2 {
namespace {
// The self-pipe. The write end is used from the signal handler, so it must be
// fully set up before the handlers are installed.
int g_shutdown_pipe_read_fd = -1;
int g_shutdown_pipe_write_fd = -1;

// Blocks on the read end of the self-pipe on a dedicated background thread and
// runs the shutdown callback when the signal handler writes to it. Created
// non-joinable and deletes itself when done, Chromium-style.
class ShutdownDetector : public base::PlatformThread::Delegate {
 public:
  ShutdownDetector(int shutdown_fd, base::RepeatingClosure shutdown_callback)
      : shutdown_fd_(shutdown_fd),
        shutdown_callback_(std::move(shutdown_callback)) {
    CHECK_NE(shutdown_fd_, -1);
    CHECK(shutdown_callback_);
  }

  ShutdownDetector(const ShutdownDetector&) = delete;
  ShutdownDetector& operator=(const ShutdownDetector&) = delete;

  void ThreadMain() override {
    base::PlatformThread::SetName("VpnShutdownDetector");

    char signaled{0};
    const ssize_t bytes_read =
        HANDLE_EINTR(read(shutdown_fd_, &signaled, sizeof(signaled)));
    if (bytes_read == static_cast<ssize_t>(sizeof(signaled))) {
      shutdown_callback_.Run();
    }
    // bytes_read == 0 means the write end was closed without a signal being
    // delivered; just let the thread exit.
    delete this;
  }

 private:
  const int shutdown_fd_;
  const base::RepeatingClosure shutdown_callback_;
};

void GracefulShutdownHandler(int sig) {
  // Reinstall the default handler.  We had one shot at graceful shutdown.
  struct sigaction action = {};
  action.sa_handler = SIG_DFL;
  RAW_CHECK(sigaction(sig, &action, nullptr) == 0);

  RAW_CHECK(g_shutdown_pipe_write_fd != -1);
  RAW_CHECK(g_shutdown_pipe_read_fd != -1);

  const char signaled{1};
  const ssize_t rv = HANDLE_EINTR(
      write(g_shutdown_pipe_write_fd, &signaled, sizeof(signaled)));
  RAW_CHECK(rv == static_cast<ssize_t>(sizeof(signaled)));
}

}  // namespace

bool ShutdownHandlers::Install() {
  CHECK(!installed_);
  installed_ = true;

  int pipefd[2];
  PCHECK(pipe(pipefd) == 0);
  g_shutdown_pipe_read_fd = pipefd[0];
  g_shutdown_pipe_write_fd = pipefd[1];

  // Deletes itself once the pipe is signaled or closed.
  auto* detector =
      new ShutdownDetector(g_shutdown_pipe_read_fd, shutdown_callback_);
  // PlatformThread does not delete its delegate.
  ANNOTATE_LEAKING_OBJECT_PTR(detector);
  CHECK(base::PlatformThread::CreateNonJoinable(0, detector));

  struct sigaction action = {};
  action.sa_handler = &GracefulShutdownHandler;
  sigemptyset(&action.sa_mask);

  // Order of signal installation is arbitrary, but if changed, make sure unit
  // tests are updated to match, since they currently rely on the last
  // registered signal being SIGHUP.
  CHECK_EQ(0, sigaction(SIGINT, &action, nullptr));
  CHECK_EQ(0, sigaction(SIGTERM, &action, nullptr));
  CHECK_EQ(0, sigaction(SIGHUP, &action, nullptr));
  return true;
}

void ShutdownHandlers::SignalShutdownComplete() {
  // No-op on POSIX: the signal handler never blocks awaiting shutdown.
}

void ShutdownHandlers::Uninstall() {
  struct sigaction action = {};
  action.sa_handler = SIG_DFL;
  sigemptyset(&action.sa_mask);
  CHECK_EQ(0, sigaction(SIGINT, &action, nullptr));
  CHECK_EQ(0, sigaction(SIGTERM, &action, nullptr));
  CHECK_EQ(0, sigaction(SIGHUP, &action, nullptr));

  // The pipe fds and the detector thread are intentionally leaked: a signal
  // delivered concurrently with this teardown may still be writing to the
  // pipe, and the detector thread may be blocked in read(). Both are
  // reclaimed by the OS at process exit, which mirrors Chromium, where the
  // equivalent handlers are never uninstalled at all.
}

}  // namespace brave_vpn::v2
