/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_IDENTITY_H_

#include "base/functional/callback.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"

namespace brave_vpn::v2 {

// FakeAgentIdentity is a client's side of agent verification with the platform
// check replaced by a fixed answer. Carries no identity channel, so
// TakeSendHandle() yields an invalid handle, which is what a platform that pins
// the peer from the transport sends anyway.
class FakeAgentIdentity : public AgentIdentity {
 public:
  explicit FakeAgentIdentity(AgentIdentity::VerificationResult result);
  ~FakeAgentIdentity() override;

  void Verify(VerificationResponseCallback callback) override;

 private:
  const AgentIdentity::VerificationResult result_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_IDENTITY_H_
