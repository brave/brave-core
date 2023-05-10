/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/environment/environment_command_line_switch_parser_util.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_ads {

absl::optional<mojom::EnvironmentType> ParseEnvironmentCommandLineSwitch() {
  const brave_rewards::RewardsFlags& flags =
      brave_rewards::RewardsFlags::ForCurrentProcess();
  if (!flags.environment) {
    return absl::nullopt;
  }

  switch (*flags.environment) {
    case brave_rewards::RewardsFlags::Environment::kDevelopment:
    case brave_rewards::RewardsFlags::Environment::kStaging: {
      return mojom::EnvironmentType::kStaging;
    }

    case brave_rewards::RewardsFlags::Environment::kProduction: {
      return mojom::EnvironmentType::kProduction;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for Environment: "
                        << static_cast<int>(*flags.environment);
}

}  // namespace brave_ads
