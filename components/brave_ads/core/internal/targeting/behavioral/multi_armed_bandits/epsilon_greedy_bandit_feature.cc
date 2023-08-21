/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"

namespace brave_ads {

BASE_FEATURE(kEpsilonGreedyBanditFeatures,
             "EpsilonGreedyBandit",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsEpsilonGreedyBanditFeatureEnabled() {
  return base::FeatureList::IsEnabled(kEpsilonGreedyBanditFeatures);
}

}  // namespace brave_ads
