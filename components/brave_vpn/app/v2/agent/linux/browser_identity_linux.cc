/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "base/files/scoped_file.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/notimplemented.h"
#include "base/process/process_handle.h"
#include "brave/components/brave_vpn/app/v2/agent/linux/browser_identity_linux_internal.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
BrowserIdentity::VerificationResult VerifyImpl(base::ProcessId /*pid*/) {
  // TODO(https://github.com/brave/brave-browser/issues/54635)
  // Implement the verification step on Linux.
  // The passed arguments are copied from the BrowserIdentity object, so this
  // function can be posted to a thread pool and run without any other context.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on Linux";
  return BrowserIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
scoped_refptr<BrowserIdentity> BrowserIdentity::Capture(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  const ucred& credentials = info.credentials;
  const pid_t pid = credentials.pid;
  if (pid <= 0) {
    return nullptr;
  }
  if (credentials.uid != geteuid()) {
    VLOG(1) << "Peer process is not running as this user, pid=" << pid;
    return nullptr;
  }

  // Residual window: SO_PEERCRED records the pid of the process that called
  // connect(), as of that call. The pidfd is opened from that pid number
  // afterwards. If the peer exited and was reaped in between and its pid was
  // handed to a new process, the pidfd, and everything read through it,
  // describe that new process instead. The uid check rules out a new process of
  // another user. One of this user's processes is not detected, and
  // verification, which inspects the pinned process, cannot catch it either.
  // The same holds when the peer hands the connected socket to another process
  // (fork, SCM_RIGHTS) and exits. Reuse needs the cyclic pid allocator to wrap
  // around pid_max within the window, which keeps it narrow. Closing it needs a
  // pidfd taken atomically with the connection (SO_PEERPIDFD), which |info|
  // does not carry.
  base::ScopedFD pidfd = internal::OpenPidfd(pid);
  if (!pidfd.is_valid()) {
    VPLOG(1) << "Cannot obtain a pidfd for the peer, pid=" << pid;
    return nullptr;
  }

  const std::optional<uint64_t> start_time_ticks =
      internal::ReadProcessStartTimeTicks(pid);
  if (!start_time_ticks) {
    VLOG(1) << "Cannot read peer start time, pid=" << pid;
    return nullptr;
  }

  // SO_PEERCRED described the process that connected; this describes the one
  // the pidfd pinned. Read before the liveness check, like the start time, so
  // it cannot belong to a later occupant of the pid.
  const std::optional<uid_t> pinned_uid =
      internal::ReadProcessEffectiveUid(pid);
  if (!pinned_uid) {
    VLOG(1) << "Cannot read peer uid, pid=" << pid;
    return nullptr;
  }
  if (*pinned_uid != geteuid()) {
    VLOG(1) << "Pinned process is not running as this user, pid=" << pid;
    return nullptr;
  }

  // A pid is only freed once its process is reaped. If the process the pidfd
  // refers to is still unreaped (zombies count) after the read, the start time
  // read above is its own and not that of a later occupant of the pid.
  if (!internal::IsProcessAlive(pidfd)) {
    VLOG(1) << "Peer process exited during capture, pid=" << pid;
    return nullptr;
  }

  return base::WrapRefCounted(new BrowserIdentity(
      pid, PlatformData{.pidfd = std::move(pidfd),
                        .start_time_ticks = *start_time_ticks}));
}

std::string BrowserIdentity::GetDescription() const {
  return absl::StrFormat("pid=%d; start_time_ticks=%u", pid_,
                         platform_data_.start_time_ticks);
}

bool BrowserIdentity::IsSameProcess(const BrowserIdentity& other) const {
  return pid_ == other.pid_ && platform_data_.start_time_ticks ==
                                   other.platform_data_.start_time_ticks;
}

BrowserIdentity::VerificationRequestCallback
BrowserIdentity::BindVerificationRequest() const {
  return base::BindOnce(&VerifyImpl, pid_);
}

}  // namespace brave_vpn::v2
