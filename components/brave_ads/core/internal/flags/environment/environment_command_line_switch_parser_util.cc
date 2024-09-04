/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/environment/environment_command_line_switch_parser_util.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_ads {

std::optional<mojom::EnvironmentType> ParseEnvironmentCommandLineSwitch() {
  const brave_rewards::RewardsFlags& rewards_flags =
      brave_rewards::RewardsFlags::ForCurrentProcess();
  if (!rewards_flags.environment) {
    return std::nullopt;
  }

  switch (*rewards_flags.environment) {
    case brave_rewards::RewardsFlags::Environment::kDevelopment:
    case brave_rewards::RewardsFlags::Environment::kStaging: {
      return mojom::EnvironmentType::kStaging;
    }

    case brave_rewards::RewardsFlags::Environment::kProduction: {
      return mojom::EnvironmentType::kProduction;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for Environment: "
                        << base::to_underlying(*rewards_flags.environment);
}

}  // namespace brave_ads
