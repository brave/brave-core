/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_unittest_helper.h"

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

EpsilonGreedyBanditHelperForTesting::EpsilonGreedyBanditHelperForTesting() =
    default;

EpsilonGreedyBanditHelperForTesting::~EpsilonGreedyBanditHelperForTesting() =
    default;

void EpsilonGreedyBanditHelperForTesting::Mock() {
  SetEpsilonGreedyBanditEligibleSegments(
      SegmentList{"architecture", "arts & entertainment", "automotive"});

  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      /*segment*/ "architecture", mojom::NotificationAdEventType::kDismissed});

  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      /*segment*/ "arts & entertainment",
      mojom::NotificationAdEventType::kDismissed});

  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      /*segment*/ "automotive", mojom::NotificationAdEventType::kClicked});
}

// static
SegmentList EpsilonGreedyBanditHelperForTesting::Expectation() {
  return {"automotive", "architecture", "arts & entertainment"};
}

}  // namespace brave_ads
