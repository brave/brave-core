/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
#define BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit_arm_info.h"

namespace ads {
namespace ad_targeting {

// TODO(Moritz Haller): raise ticket for place to put shared data structures

// TODO use map instead of vector
using EpsilonGreedyBanditArmList = std::vector<EpsilonGreedyBanditArmInfo>;

class EpsilonGreedyBanditArms {
 public:
  EpsilonGreedyBanditArms();

  ~EpsilonGreedyBanditArms();

  static EpsilonGreedyBanditArmList FromJson(
      const std::string& json);  // TODO(Moritz Haller): Make const if not static NOLINT

  static std::string ToJson(
      const EpsilonGreedyBanditArmList& arms);  // TODO(Moritz Haller): Make const if not static NOLINT

 private:
  // TODO(Moritz Haller): private static? better have free functions?
  static EpsilonGreedyBanditArmList GetFromList(
      const base::ListValue* list);  // TODO(Moritz Haller): Make const if not static NOLINT

  static bool GetFromDictionary(
      const base::DictionaryValue* dictionary,
      EpsilonGreedyBanditArmInfo* info);  // TODO(Moritz Haller): Make const if not static NOLINT
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_BANDITS_EPSILON_GREEDY_BANDIT_ARMS_H_
