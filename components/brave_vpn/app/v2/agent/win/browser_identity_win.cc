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

class BrowserIdentityWin : public BrowserIdentity {
 public:
  explicit BrowserIdentityWin(base::ProcessId pid) : pid_(pid) {}

  BrowserIdentityWin(const BrowserIdentityWin&) = delete;
  BrowserIdentityWin& operator=(const BrowserIdentityWin&) = delete;

  // BrowserIdentity overrides:
  base::ProcessId pid() const override { return pid_; }

  std::string GetDescription() const override {
    return absl::StrFormat("pid=%lu", static_cast<unsigned long>(pid_));
  }

  BrowserIdentity::VerificationResult Verify() const override {
    // TODO(https://github.com/brave/brave-browser/issues/54633)
    // Implement the verification step on Windows.
    NOTIMPLEMENTED()
        << "Identity verification is not yet implemented on Windows";
    return VerificationResult::kAccepted;
  }

  bool IsSameProcess(const BrowserIdentity& /*other*/) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54633)
    // Implement the comparison step on Windows: check if the process this
    // object names is the same as the one |other| names.
    NOTIMPLEMENTED() << "IsSameProcess is not yet implemented on Windows";
    return true;
  }

 private:
  ~BrowserIdentityWin() override = default;

  const base::ProcessId pid_;
};

class BrowserIdentityFactoryWin : public BrowserIdentityFactory {
 public:
  // BrowserIdentityFactory overrides:
  scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info) const override {
    // TODO(https://github.com/brave/brave-browser/issues/54633)
    // Implement the capture step on Windows: pin the peer process and take the
    // platform-specific data BrowserIdentityWin needs. Returning null here
    // refuses the connection outright.
    NOTIMPLEMENTED() << "Identity capture is not yet implemented on Windows";
    return base::MakeRefCounted<BrowserIdentityWin>(info.pid);
  }
};

}  // namespace

std::unique_ptr<BrowserIdentityFactory> CreateBrowserIdentityFactory() {
  return std::make_unique<BrowserIdentityFactoryWin>();
}

}  // namespace brave_vpn::v2
