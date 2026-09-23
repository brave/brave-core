/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_LAUNCHER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_TEST_FAKE_AGENT_LAUNCHER_H_

#include <stddef.h>

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

namespace brave_vpn::v2 {

class FakeAgentLauncher : public AgentLauncher {
 public:
  struct Record {
    Record();
    ~Record();

    Record(const Record&) = delete;
    Record& operator=(const Record&) = delete;

    size_t launch_count() const { return failure_callbacks.size(); }

    AgentLauncher::LaunchFailureCallback TakeFailureCallback(size_t index) {
      CHECK_LT(index, failure_callbacks.size());
      CHECK(failure_callbacks[index]);
      return std::move(failure_callbacks[index]);
    }

    std::vector<AgentLauncher::LaunchFailureCallback> failure_callbacks;
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
