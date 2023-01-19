/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_util.h"

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_value_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::targeting {

void SetEpsilonGreedyBanditArms(const EpsilonGreedyBanditArmMap& arms) {
  AdsClientHelper::GetInstance()->SetDictPref(
      prefs::kEpsilonGreedyBanditArms, EpsilonGreedyBanditArmsToValue(arms));
}

EpsilonGreedyBanditArmMap GetEpsilonGreedyBanditArms() {
  const absl::optional<base::Value::Dict> dict =
      AdsClientHelper::GetInstance()->GetDictPref(
          prefs::kEpsilonGreedyBanditArms);
  if (!dict) {
    return {};
  }

  return EpsilonGreedyBanditArmsFromValue(*dict);
}

}  // namespace ads::targeting
