/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_

#include <string>
#include <vector>

namespace brave_ads {

struct EpsilonGreedyBanditArmInfo final {
  bool operator==(const EpsilonGreedyBanditArmInfo&) const;
  bool operator!=(const EpsilonGreedyBanditArmInfo&) const;

  [[nodiscard]] bool IsValid() const;

  std::string segment;
  double value = 0.0;
  int pulls = 0;
};

using EpsilonGreedyBanditArmList = std::vector<EpsilonGreedyBanditArmInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_
