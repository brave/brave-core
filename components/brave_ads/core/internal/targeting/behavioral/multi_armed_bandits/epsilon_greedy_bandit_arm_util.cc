/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_value_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

void SetEpsilonGreedyBanditArms(const EpsilonGreedyBanditArmMap& arms) {
  SetProfileDictPref(prefs::kEpsilonGreedyBanditArms,
                     EpsilonGreedyBanditArmsToValue(arms));
}

void ResetEpsilonGreedyBanditArms() {
  ClearProfilePref(prefs::kEpsilonGreedyBanditArms);
}

EpsilonGreedyBanditArmMap GetEpsilonGreedyBanditArms() {
  const absl::optional<base::Value::Dict> dict =
      GetProfileDictPref(prefs::kEpsilonGreedyBanditArms);
  if (!dict) {
    return {};
  }

  return EpsilonGreedyBanditArmsFromValue(*dict);
}

}  // namespace brave_ads
