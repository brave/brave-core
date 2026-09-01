/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/test/fake_agent_launcher.h"

#include <utility>

namespace brave_vpn::v2 {

FakeAgentLauncher::FakeAgentLauncher(Record* record) : record_(record) {}

FakeAgentLauncher::~FakeAgentLauncher() = default;

void FakeAgentLauncher::Launch(LaunchFailureCallback failure_callback) {
  ++record_->launch_count;
  record_->last_failure_callback = std::move(failure_callback);
}

}  // namespace brave_vpn::v2
