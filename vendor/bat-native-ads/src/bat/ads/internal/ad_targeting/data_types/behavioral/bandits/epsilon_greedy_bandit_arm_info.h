/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_

#include <string>

namespace ads {
namespace ad_targeting {

struct EpsilonGreedyBanditArmInfo {
  EpsilonGreedyBanditArmInfo();

  EpsilonGreedyBanditArmInfo(const EpsilonGreedyBanditArmInfo& info);

  ~EpsilonGreedyBanditArmInfo();

  bool operator==(const EpsilonGreedyBanditArmInfo& rhs) const;
  bool operator!=(const EpsilonGreedyBanditArmInfo& rhs) const;

  bool IsValid() const;

  std::string segment;
  double value = 0.0;
  int pulls = 0;
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_INFO_H_
