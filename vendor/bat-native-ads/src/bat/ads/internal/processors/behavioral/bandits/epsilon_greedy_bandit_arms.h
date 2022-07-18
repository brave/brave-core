/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_

#include <string>

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms_alias.h"

namespace ads {
namespace targeting {

class EpsilonGreedyBanditArms final {
 public:
  EpsilonGreedyBanditArms();
  ~EpsilonGreedyBanditArms();
  EpsilonGreedyBanditArms(const EpsilonGreedyBanditArms&) = delete;
  EpsilonGreedyBanditArms& operator=(const EpsilonGreedyBanditArms&) = delete;

  static EpsilonGreedyBanditArmMap FromJson(const std::string& json);
  static std::string ToJson(const EpsilonGreedyBanditArmMap& arms);
};

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
