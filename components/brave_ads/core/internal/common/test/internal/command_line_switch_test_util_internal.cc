/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"

#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_ads::test {

void SimulateCommandLineSwitches() {
  brave_rewards::RewardsFlags::SetForceParsingForTesting(true);
}

void ResetCommandLineSwitches() {
  brave_rewards::RewardsFlags::SetForceParsingForTesting(false);
}

std::optional<bool>& DidAppendCommandLineSwitches() {
  static std::optional<bool> did_append;
  return did_append;
}

}  // namespace brave_ads::test
