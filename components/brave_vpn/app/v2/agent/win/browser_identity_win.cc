/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <windows.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/notimplemented.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/win/access_token.h"
#include "base/win/sid.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
bool IsRunningAsCurrentUser(HANDLE process) {
  const std::optional<base::win::AccessToken> peer =
      base::win::AccessToken::FromProcess(process);
  const std::optional<base::win::AccessToken> self =
      base::win::AccessToken::FromCurrentProcess();
  return peer && self && peer->User() == self->User();
}

BrowserIdentity::VerificationResult VerifyImpl(base::ProcessId /*pid*/) {
  // TODO(https://github.com/brave/brave-browser/issues/54633)
  // Implement the verification step on Windows.
  // The passed arguments are copied from the BrowserIdentity object, so this
  // function can be posted to a thread pool and run without any other context.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on Windows";
  return BrowserIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
scoped_refptr<BrowserIdentity> BrowserIdentity::Capture(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  const base::ProcessId pid = info.pid;
  if (pid == base::kNullProcessId) {
    return nullptr;
  }

  // The server opened |info.process| while accepting the connection, and
  // holding it keeps |pid| from being reused while this identity lives. The
  // server read the pid from the pipe before opening it, though, so a peer
  // that exited and had its pid recycled in that gap would be pinned instead;
  // the user and session checks narrow that window.
  if (!info.process.IsValid()) {
    VLOG(1) << "Peer process handle is not valid, pid=" << pid;
    return nullptr;
  }

  DWORD own_session_id = 0;
  // Cannot fail when querying the calling process's own id.
  CHECK(::ProcessIdToSessionId(::GetCurrentProcessId(), &own_session_id));
  if (info.session_id != own_session_id) {
    VLOG(1) << "Peer process is in another session, pid=" << pid;
    return nullptr;
  }

  base::Process process = info.process.Duplicate();
  if (!process.IsValid()) {
    VPLOG(1) << "Cannot duplicate peer process handle, pid=" << pid;
    return nullptr;
  }
  if (process.Pid() != pid) {
    VLOG(1) << "Peer process handle does not match pid=" << pid;
    return nullptr;
  }

  DWORD exit_code = 0;
  if (!::GetExitCodeProcess(process.Handle(), &exit_code) ||
      exit_code != STILL_ACTIVE) {
    VLOG(1) << "Peer process already exited, pid=" << pid;
    return nullptr;
  }

  if (!IsRunningAsCurrentUser(process.Handle())) {
    VLOG(1) << "Peer process is not running as this user, pid=" << pid;
    return nullptr;
  }

  FILETIME creation_filetime{}, exit_filetime{}, kernel_filetime{},
      user_filetime{};
  if (!::GetProcessTimes(process.Handle(), &creation_filetime, &exit_filetime,
                         &kernel_filetime, &user_filetime)) {
    VPLOG(1) << "Cannot read peer process times, pid=" << pid;
    return nullptr;
  }
  const uint64_t creation_time =
      (static_cast<uint64_t>(creation_filetime.dwHighDateTime) << 32) |
      creation_filetime.dwLowDateTime;

  return base::WrapRefCounted(
      new BrowserIdentity(pid, PlatformData{.process = std::move(process),
                                            .creation_time = creation_time}));
}

std::string BrowserIdentity::GetDescription() const {
  const auto pid = static_cast<unsigned long>(pid_);
  if (!platform_data_) {
    return absl::StrFormat("pid=%u", pid);
  }
  return absl::StrFormat("pid=%u; creation_time=%u", pid,
                         platform_data_->creation_time);
}

bool BrowserIdentity::IsSameProcess(const BrowserIdentity& other) const {
  if (pid_ != other.pid_ || !platform_data_ || !other.platform_data_) {
    return false;
  }
  return platform_data_->creation_time == other.platform_data_->creation_time;
}

BrowserIdentity::VerificationRequestCallback
BrowserIdentity::BindVerificationRequest() const {
  return base::BindOnce(&VerifyImpl, pid_);
}

}  // namespace brave_vpn::v2
