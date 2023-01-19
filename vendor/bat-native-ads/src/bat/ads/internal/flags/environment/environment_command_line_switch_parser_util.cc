/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/environment/environment_command_line_switch_parser_util.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace ads {

absl::optional<EnvironmentType> ParseEnvironmentCommandLineSwitch() {
  const brave_rewards::RewardsFlags& flags =
      brave_rewards::RewardsFlags::ForCurrentProcess();
  if (!flags.environment) {
    return absl::nullopt;
  }

  switch (*flags.environment) {
    case brave_rewards::RewardsFlags::Environment::kDevelopment:
    case brave_rewards::RewardsFlags::Environment::kStaging: {
      return EnvironmentType::kStaging;
    }

    case brave_rewards::RewardsFlags::Environment::kProduction: {
      return EnvironmentType::kProduction;
    }
  }

  NOTREACHED() << "Unexpected value for Environment: "
               << static_cast<int>(*flags.environment);
  return absl::nullopt;
}

}  // namespace ads
