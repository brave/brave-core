/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <bsm/libbsm.h>

#include <string>

#include "base/functional/bind.h"
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
  NOTIMPLEMENTED() << "Identity verification is not yet implemented on macOS";
  return BrowserIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
scoped_refptr<BrowserIdentity> BrowserIdentity::Capture(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  // TODO(https://github.com/brave/brave-browser/issues/54634)
  // Implement the capture step on macOS: pin the peer process and take the
  // platform-specific data BrowserIdentity needs. Returning null here refuses
  // the connection outright.
  NOTIMPLEMENTED() << "Identity capture is not yet implemented on macOS";
  return base::WrapRefCounted(
      new BrowserIdentity(audit_token_to_pid(info.audit_token)));
}

std::string BrowserIdentity::GetDescription() const {
  return absl::StrFormat("pid=%d", pid_);
}

bool BrowserIdentity::IsSameProcess(const BrowserIdentity& /*other*/) const {
  // TODO(https://github.com/brave/brave-browser/issues/54634)
  // Implement the comparison step on macOS: check if the process this object
  // names is the same as the one |other| names.
  NOTIMPLEMENTED() << "IsSameProcess is not yet implemented on macOS";
  return true;
}

BrowserIdentity::VerificationRequestCallback
BrowserIdentity::BindVerificationRequest() const {
  return base::BindOnce(&VerifyImpl, pid_);
}

}  // namespace brave_vpn::v2
