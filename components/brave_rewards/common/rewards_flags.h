/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards {

struct RewardsFlags {
  enum class Environment { kDevelopment, kStaging, kProduction };

  static void SetForceParsingForTesting(bool force_parsing_for_testing);

  static const std::string& GetCommandLineSwitchASCII();

  static const RewardsFlags& ForCurrentProcess();

  absl::optional<Environment> environment;
  bool debug = false;
  bool persist_logs = false;
  absl::optional<int> reconcile_interval;
  absl::optional<int> retry_interval;
  absl::optional<int> gemini_retries;
  absl::optional<int> country_id;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_FLAGS_H_
