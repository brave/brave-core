/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/notimplemented.h"
#include "base/process/process_handle.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {

class BrowserIdentityLinux : public BrowserIdentity {
 public:
  explicit BrowserIdentityLinux(base::ProcessId pid) : pid_(pid) {}

  BrowserIdentityLinux(const BrowserIdentityLinux&) = delete;
  BrowserIdentityLinux& operator=(const BrowserIdentityLinux&) = delete;

  // BrowserIdentity overrides:
  base::ProcessId pid() const override { return pid_; }

  std::string GetDescription() const override {
    return absl::StrFormat("pid=%d", pid_);
  }

  BrowserIdentity::VerificationResult Verify() const override {
    // TODO(https://github.com/brave/brave-browser/issues/54635)
    // Implement the verification step on Linux.
    NOTIMPLEMENTED() << "Identity verification is not yet implemented on Linux";
    return VerificationResult::kAccepted;
  }

  bool IsSameProcess(const BrowserIdentity& /*other*/) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54635)
    // Implement the comparison step on Linux: check if the process this object
    // names is the same as the one |other| names.
    NOTIMPLEMENTED() << "IsSameProcess is not yet implemented on Linux";
    return true;
  }

 private:
  ~BrowserIdentityLinux() override = default;

  const base::ProcessId pid_;
};

class BrowserIdentityFactoryLinux : public BrowserIdentityFactory {
 public:
  // BrowserIdentityFactory overrides:
  scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54635)
    // Implement the capture step on Linux: pin the peer process and take the
    // platform-specific data BrowserIdentityLinux needs. Returning null here
    // refuses the connection outright.
    NOTIMPLEMENTED() << "Identity capture is not yet implemented on Linux";
    return base::MakeRefCounted<BrowserIdentityLinux>(info.credentials.pid);
  }
};

}  // namespace

std::unique_ptr<BrowserIdentityFactory> CreateBrowserIdentityFactory() {
  return std::make_unique<BrowserIdentityFactoryLinux>();
}

}  // namespace brave_vpn::v2
