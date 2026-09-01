/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_LAUNCHER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_LAUNCHER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

namespace brave_vpn::v2 {

class FakeAgentLauncher : public AgentLauncher {
 public:
  struct Record {
    int launch_count = 0;
    AgentLauncher::LaunchFailureCallback last_failure_callback;
  };

  explicit FakeAgentLauncher(Record* record);
  ~FakeAgentLauncher() override;

  // AgentLauncher overrides:
  void Launch(LaunchFailureCallback failure_callback) override;

 private:
  const raw_ptr<Record> record_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_LAUNCHER_H_
