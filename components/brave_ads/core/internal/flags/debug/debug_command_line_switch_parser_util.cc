/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/debug/debug_command_line_switch_parser_util.h"

#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_ads {

bool ParseDebugCommandLineSwitch() {
  return brave_rewards::RewardsFlags::ForCurrentProcess().debug;
}

}  // namespace brave_ads
