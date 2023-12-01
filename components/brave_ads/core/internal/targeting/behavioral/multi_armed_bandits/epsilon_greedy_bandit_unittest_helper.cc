/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_unittest_helper.h"

#include <string>

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_segments.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads::test {

EpsilonGreedyBanditHelper::EpsilonGreedyBanditHelper() = default;

EpsilonGreedyBanditHelper::~EpsilonGreedyBanditHelper() = default;

void EpsilonGreedyBanditHelper::Mock() {
  SetEpsilonGreedyBanditEligibleSegments(
      SupportedEpsilonGreedyBanditSegments());

  // Set all values to zero by choosing a zero-reward action due to optimistic
  // initial values for arms.
  for (const std::string& segment : SupportedEpsilonGreedyBanditSegments()) {
    processor_.Process(EpsilonGreedyBanditFeedbackInfo{
        std::string(segment), mojom::NotificationAdEventType::kDismissed});
  }

  const std::string segment_1 = "science";
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_1, mojom::NotificationAdEventType::kClicked});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_1, mojom::NotificationAdEventType::kClicked});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_1, mojom::NotificationAdEventType::kClicked});

  const std::string segment_2 = "travel";
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_2, mojom::NotificationAdEventType::kDismissed});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_2, mojom::NotificationAdEventType::kClicked});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_2, mojom::NotificationAdEventType::kClicked});

  const std::string segment_3 = "technology & computing";
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_3, mojom::NotificationAdEventType::kDismissed});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_3, mojom::NotificationAdEventType::kDismissed});
  processor_.Process(EpsilonGreedyBanditFeedbackInfo{
      segment_3, mojom::NotificationAdEventType::kClicked});
}

// static
SegmentList EpsilonGreedyBanditHelper::Expectation() {
  return {"science", "travel", "technology & computing"};
}

}  // namespace brave_ads::test
