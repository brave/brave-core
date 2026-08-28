/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_identity.h"

#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {

class FakeBrowserIdentity : public BrowserIdentity {
 public:
  FakeBrowserIdentity(base::ProcessId pid,
                      scoped_refptr<FakeBrowserIdentitySettings> settings)
      : pid_(pid), settings_(std::move(settings)) {}

  FakeBrowserIdentity(const FakeBrowserIdentity&) = delete;
  FakeBrowserIdentity& operator=(const FakeBrowserIdentity&) = delete;

  // BrowserIdentity overrides:
  base::ProcessId pid() const override { return pid_; }

  std::string GetDescription() const override {
    return absl::StrFormat("fake pid=%d", static_cast<int>(pid_));
  }

  VerificationResult Verify() const override {
    return settings_->verification_result();
  }

  bool IsSameProcess(const BrowserIdentity& /*other*/) const override {
    return settings_->is_same_process();
  }

 private:
  ~FakeBrowserIdentity() override = default;

  const base::ProcessId pid_;
  const scoped_refptr<FakeBrowserIdentitySettings> settings_;
};

}  // namespace

FakeBrowserIdentitySettings::FakeBrowserIdentitySettings() = default;

FakeBrowserIdentitySettings::~FakeBrowserIdentitySettings() = default;

FakeBrowserIdentityFactory::FakeBrowserIdentityFactory()
    : settings_(base::MakeRefCounted<FakeBrowserIdentitySettings>()) {}

FakeBrowserIdentityFactory::~FakeBrowserIdentityFactory() = default;

scoped_refptr<BrowserIdentity> FakeBrowserIdentityFactory::Capture(
    const named_mojo_ipc_server::ConnectionInfo& /*info*/) const {
  if (capture_fails_) {
    return nullptr;
  }
  return base::MakeRefCounted<FakeBrowserIdentity>(peer_pid_, settings_);
}

}  // namespace brave_vpn::v2
