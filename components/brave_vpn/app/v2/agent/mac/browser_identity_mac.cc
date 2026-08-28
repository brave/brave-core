/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <bsm/libbsm.h>

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/notimplemented.h"
#include "base/process/process_handle.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {

class BrowserIdentityMac : public BrowserIdentity {
 public:
  explicit BrowserIdentityMac(base::ProcessId pid) : pid_(pid) {}

  BrowserIdentityMac(const BrowserIdentityMac&) = delete;
  BrowserIdentityMac& operator=(const BrowserIdentityMac&) = delete;

  // BrowserIdentity overrides:
  base::ProcessId pid() const override { return pid_; }

  std::string GetDescription() const override {
    return absl::StrFormat("pid=%d", pid_);
  }

  BrowserIdentity::VerificationResult Verify() const override {
    // TODO(https://github.com/brave/brave-browser/issues/54634)
    // Implement the verification step on macOS.
    NOTIMPLEMENTED() << "Identity verification is not yet implemented on macOS";
    return VerificationResult::kAccepted;
  }

  bool IsSameProcess(const BrowserIdentity& /*other*/) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54634)
    // Implement the comparison step on macOS: check if the process this object
    // names is the same as the one |other| names.
    NOTIMPLEMENTED() << "IsSameProcess is not yet implemented on macOS";
    return true;
  }

 private:
  ~BrowserIdentityMac() override = default;

  const base::ProcessId pid_;
};

class BrowserIdentityFactoryMac : public BrowserIdentityFactory {
 public:
  // BrowserIdentityFactory overrides:
  scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54634)
    // Implement the capture step on macOS: pin the peer process and take the
    // platform-specific data BrowserIdentityMac needs. Returning null here
    // refuses the connection outright.
    NOTIMPLEMENTED() << "Identity capture is not yet implemented on macOS";
    return base::MakeRefCounted<BrowserIdentityMac>(
        audit_token_to_pid(info.audit_token));
  }
};

}  // namespace

std::unique_ptr<BrowserIdentityFactory> CreateBrowserIdentityFactory() {
  return std::make_unique<BrowserIdentityFactoryMac>();
}

}  // namespace brave_vpn::v2
