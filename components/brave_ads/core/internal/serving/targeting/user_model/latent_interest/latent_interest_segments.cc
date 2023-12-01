/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_segments.h"

#include "base/containers/extend.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/model/epsilon_greedy_bandit_model.h"

namespace brave_ads {

SegmentList BuildLatentInterestSegments() {
  SegmentList segments;

  if (base::FeatureList::IsEnabled(kEpsilonGreedyBanditFeature)) {
    base::Extend(segments, GetEpsilonGreedyBanditSegments());
  }

  return segments;
}

}  // namespace brave_ads
