/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_

#include <string>

#include "base/functional/callback.h"
#include "base/process/process_handle.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

namespace brave_vpn::v2 {

// FakeBrowserIdentity is a fake identity which describes whatever
// peer the test says is connecting. ConnectionInfo is never read, which is the
// point: a test can name any pid and choose any verdict without fabricating the
// kernel credentials each platform carries them in.
class FakeBrowserIdentity : public BrowserIdentity {
 public:
  using FakeVerificationResultProvider =
      base::RepeatingCallback<BrowserIdentity::VerificationResult()>;

  FakeBrowserIdentity(base::ProcessId pid,
                      FakeVerificationResultProvider result_provider,
                      bool is_same_process);

  // BrowserIdentity overrides:
  std::string GetDescription() const override;
  bool IsSameProcess(const BrowserIdentity& other) const override;
  void Verify(VerificationResponseCallback callback) const override;

 private:
  ~FakeBrowserIdentity() override;

  FakeVerificationResultProvider verification_result_;
  bool is_same_process_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_IDENTITY_H_
