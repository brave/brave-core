/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_

#include <string>

#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_arms_aliases.h"

namespace ads {
namespace ad_targeting {

class EpsilonGreedyBanditArms final {
 public:
  EpsilonGreedyBanditArms();
  ~EpsilonGreedyBanditArms();

  static EpsilonGreedyBanditArmMap FromJson(const std::string& json);
  static std::string ToJson(const EpsilonGreedyBanditArmMap& arms);
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
