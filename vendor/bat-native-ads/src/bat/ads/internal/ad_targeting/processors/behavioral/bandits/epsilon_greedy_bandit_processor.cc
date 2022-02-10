/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"

#include <algorithm>

#include "base/check.h"
#include "base/notreached.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_arms.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/segments/segments_util.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace ad_targeting {
namespace processor {

namespace {

const double kArmDefaultValue = 1.0;
const uint64_t kArmDefaultPulls = 0;

EpsilonGreedyBanditArmMap MaybeAddOrResetArms(
    const EpsilonGreedyBanditArmMap& arms) {
  EpsilonGreedyBanditArmMap updated_arms = arms;

  for (const auto& segment : kSegments) {
    const auto iter = updated_arms.find(segment);
    if (iter != updated_arms.end()) {
      const EpsilonGreedyBanditArmInfo arm = iter->second;
      if (arm.IsValid()) {
        BLOG(3, "Epsilon greedy bandit arm already exists for " << segment
                                                                << " segment");

        continue;
      }
    }

    EpsilonGreedyBanditArmInfo arm;
    arm.value = kArmDefaultValue;
    arm.pulls = kArmDefaultPulls;

    updated_arms[segment] = arm;

    BLOG(2,
         "Epsilon greedy bandit arm was added for " << segment << " segment");
  }

  return updated_arms;
}

EpsilonGreedyBanditArmMap MaybeDeleteArms(
    const EpsilonGreedyBanditArmMap& arms) {
  EpsilonGreedyBanditArmMap updated_arms = arms;

  for (auto arm_iter = updated_arms.begin(); arm_iter != updated_arms.end();) {
    const auto segment_iter =
        std::find(kSegments.cbegin(), kSegments.cend(), arm_iter->first);
    if (segment_iter != kSegments.end()) {
      ++arm_iter;
      continue;
    }

    BLOG(2, "Epsilon greedy bandit arm was deleted for " << arm_iter->first
                                                         << " segment ");

    arm_iter = updated_arms.erase(arm_iter);
  }

  return updated_arms;
}

}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() {
  InitializeArms();
}

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

void EpsilonGreedyBandit::Process(const BanditFeedbackInfo& feedback) {
  DCHECK(!feedback.segment.empty());

  const std::string segment = GetParentSegment(feedback.segment);
  DCHECK(!segment.empty());

  switch (feedback.ad_event_type) {
    case mojom::AdNotificationEventType::kTimedOut:
    case mojom::AdNotificationEventType::kDismissed: {
      UpdateArm(/* reward */ 0, segment);
      break;
    }

    case mojom::AdNotificationEventType::kClicked: {
      UpdateArm(/* reward */ 1, segment);
      break;
    }

    case mojom::AdNotificationEventType::kServed:
    case mojom::AdNotificationEventType::kViewed: {
      NOTREACHED();
      break;
    }
  }

  BLOG(1, "Epsilon greedy bandit processed " << feedback.ad_event_type);
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBandit::InitializeArms() const {
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);

  EpsilonGreedyBanditArmMap arms = EpsilonGreedyBanditArms::FromJson(json);

  arms = MaybeAddOrResetArms(arms);

  arms = MaybeDeleteArms(arms);

  json = EpsilonGreedyBanditArms::ToJson(arms);
  AdsClientHelper::Get()->SetStringPref(prefs::kEpsilonGreedyBanditArms, json);

  BLOG(1, "Successfully initialized epsilon greedy bandit arms");
}

void EpsilonGreedyBandit::UpdateArm(const uint64_t reward,
                                    const std::string& segment) const {
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);

  EpsilonGreedyBanditArmMap arms = EpsilonGreedyBanditArms::FromJson(json);

  if (arms.empty()) {
    BLOG(1, "No epsilon greedy bandit arms");
    return;
  }

  const auto iter = arms.find(segment);
  if (iter == arms.end()) {
    BLOG(1, "Epsilon greedy bandit arm was not found for " << segment
                                                           << " segment");
    return;
  }

  EpsilonGreedyBanditArmInfo arm = iter->second;
  arm.pulls++;
  arm.value = arm.value + (1.0 / arm.pulls * (reward - arm.value));
  iter->second = arm;

  json = EpsilonGreedyBanditArms::ToJson(arms);

  AdsClientHelper::Get()->SetStringPref(prefs::kEpsilonGreedyBanditArms, json);

  BLOG(1,
       "Epsilon greedy bandit arm was updated for " << segment << " segment");
}

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads
