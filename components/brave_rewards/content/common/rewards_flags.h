/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_

#include <optional>
#include <string>


namespace brave_rewards {

struct RewardsFlags {
  enum class Environment { kDevelopment, kStaging, kProduction };

  static void SetForceParsingForTesting(bool force_parsing_for_testing);

  static const std::string& GetCommandLineSwitchASCII();

  static const RewardsFlags& ForCurrentProcess();

  std::optional<Environment> environment;
  bool debug = false;
  bool persist_logs = false;
  std::optional<int> reconcile_interval;
  std::optional<int> retry_interval;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_
