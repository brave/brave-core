/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <bsm/libbsm.h>
#include <unistd.h>

#include <string>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/notimplemented.h"
#include "base/process/process_handle.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
BrowserIdentity::VerificationResult VerifyImpl(base::ProcessId /*pid*/) {
  // TODO(https://github.com/brave/brave-browser/issues/54634)
  // Implement the verification step on macOS.
  // The passed arguments are copied from the BrowserIdentity object, so this
  // function can be posted to a thread pool and run without any other context.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on macOS";
  return BrowserIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
scoped_refptr<BrowserIdentity> BrowserIdentity::Capture(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  // The audit token was taken by the kernel together with the connection, and
  // its pidversion distinguishes this process instance from any later one that
  // reuses the pid, so the token alone pins the peer.
  const audit_token_t& token = info.audit_token;
  const pid_t pid = audit_token_to_pid(token);
  if (pid <= 0) {
    return nullptr;
  }

  if (audit_token_to_euid(token) != geteuid()) {
    VLOG(1) << "Peer process is not running as this user, pid=" << pid;
    return nullptr;
  }

  return base::WrapRefCounted(
      new BrowserIdentity(pid, PlatformData{.audit_token = token}));
}

std::string BrowserIdentity::GetDescription() const {
  return absl::StrFormat("pid=%d; pidversion=%d", pid_,
                         audit_token_to_pidversion(platform_data_.audit_token));
}

bool BrowserIdentity::IsSameProcess(const BrowserIdentity& other) const {
  return pid_ == other.pid_ &&
         audit_token_to_pidversion(platform_data_.audit_token) ==
             audit_token_to_pidversion(other.platform_data_.audit_token);
}

BrowserIdentity::VerificationRequestCallback
BrowserIdentity::BindVerificationRequest() const {
  return base::BindOnce(&VerifyImpl, pid_);
}

}  // namespace brave_vpn::v2
