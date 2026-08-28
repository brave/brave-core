/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_

#include <atomic>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

namespace named_mojo_ipc_server {
struct ConnectionInfo;
}  // namespace named_mojo_ipc_server

namespace brave_vpn::v2 {

// FakeBrowserIdentitySettings encapsulates the answers the fake identities
// give, shared by the factory and every identity it has handed out. Setting one
// changes what an identity the registry is already holding will answer next.
class FakeBrowserIdentitySettings
    : public base::RefCountedThreadSafe<FakeBrowserIdentitySettings> {
 public:
  FakeBrowserIdentitySettings();

  FakeBrowserIdentitySettings(const FakeBrowserIdentitySettings&) = delete;
  FakeBrowserIdentitySettings& operator=(const FakeBrowserIdentitySettings&) =
      delete;

  BrowserIdentity::VerificationResult verification_result() const {
    return verification_result_.load(std::memory_order_relaxed);
  }
  void set_verification_result(BrowserIdentity::VerificationResult result) {
    verification_result_.store(result, std::memory_order_relaxed);
  }

  bool is_same_process() const {
    return is_same_process_.load(std::memory_order_relaxed);
  }
  void set_is_same_process(bool same) {
    is_same_process_.store(same, std::memory_order_relaxed);
  }

 private:
  friend class base::RefCountedThreadSafe<FakeBrowserIdentitySettings>;

  ~FakeBrowserIdentitySettings();

  std::atomic<BrowserIdentity::VerificationResult> verification_result_{
      BrowserIdentity::VerificationResult::kAccepted};
  std::atomic<bool> is_same_process_{true};
};

// FakeBrowserIdentityFactory creates a fake identity which describes whatever
// peer the test says is connecting. ConnectionInfo is never read, which is the
// point: a test can name any pid and choose any verdict without fabricating the
// kernel credentials each platform carries them in.
class FakeBrowserIdentityFactory : public BrowserIdentityFactory {
 public:
  FakeBrowserIdentityFactory();
  ~FakeBrowserIdentityFactory() override;

  void set_peer_pid(base::ProcessId pid) { peer_pid_ = pid; }
  void set_capture_fails(bool fails) { capture_fails_ = fails; }
  void set_verification_result(BrowserIdentity::VerificationResult result) {
    settings_->set_verification_result(result);
  }
  void set_is_same_process(bool same) { settings_->set_is_same_process(same); }

  // BrowserIdentityFactory overrides:
  scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info) const override;

 private:
  const scoped_refptr<FakeBrowserIdentitySettings> settings_;
  base::ProcessId peer_pid_ = 1234;
  bool capture_fails_ = false;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_
