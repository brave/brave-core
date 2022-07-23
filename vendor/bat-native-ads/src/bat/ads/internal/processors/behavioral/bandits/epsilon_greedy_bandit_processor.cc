/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"

#include <algorithm>

#include "base/check.h"
#include "base/notreached.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/segments/segment_util.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace processor {

namespace {

constexpr double kArmDefaultValue = 1.0;
constexpr uint64_t kArmDefaultPulls = 0;

targeting::EpsilonGreedyBanditArmMap MaybeAddOrResetArms(
    const targeting::EpsilonGreedyBanditArmMap& arms) {
  targeting::EpsilonGreedyBanditArmMap updated_arms = arms;

  for (const auto& segment : targeting::kSegments) {
    const auto iter = updated_arms.find(segment);
    if (iter != updated_arms.end()) {
      const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
      if (arm.IsValid()) {
        BLOG(3, "Epsilon greedy bandit arm already exists for " << segment
                                                                << " segment");

        continue;
      }
    }

    targeting::EpsilonGreedyBanditArmInfo arm;
    arm.value = kArmDefaultValue;
    arm.pulls = kArmDefaultPulls;

    updated_arms[segment] = arm;

    BLOG(2,
         "Epsilon greedy bandit arm was added for " << segment << " segment");
  }

  return updated_arms;
}

targeting::EpsilonGreedyBanditArmMap MaybeDeleteArms(
    const targeting::EpsilonGreedyBanditArmMap& arms) {
  targeting::EpsilonGreedyBanditArmMap updated_arms = arms;

  for (auto arm_iter = updated_arms.begin(); arm_iter != updated_arms.end();) {
    const auto segment_iter =
        std::find(targeting::kSegments.cbegin(), targeting::kSegments.cend(),
                  arm_iter->first);
    if (segment_iter != targeting::kSegments.end()) {
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

  const mojom::NotificationAdEventType ad_event_type = feedback.ad_event_type;
  DCHECK(mojom::IsKnownEnumValue(ad_event_type));
  switch (ad_event_type) {
    case mojom::NotificationAdEventType::kTimedOut:
    case mojom::NotificationAdEventType::kDismissed: {
      UpdateArm(/* reward */ 0, segment);
      break;
    }

    case mojom::NotificationAdEventType::kClicked: {
      UpdateArm(/* reward */ 1, segment);
      break;
    }

    case mojom::NotificationAdEventType::kServed:
    case mojom::NotificationAdEventType::kViewed: {
      NOTREACHED();
      break;
    }
  }

  BLOG(1, "Epsilon greedy bandit processed " << feedback.ad_event_type);
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBandit::InitializeArms() const {
  std::string json = AdsClientHelper::GetInstance()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);

  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  arms = MaybeAddOrResetArms(arms);

  arms = MaybeDeleteArms(arms);

  json = targeting::EpsilonGreedyBanditArms::ToJson(arms);
  AdsClientHelper::GetInstance()->SetStringPref(prefs::kEpsilonGreedyBanditArms,
                                                json);

  BLOG(1, "Successfully initialized epsilon greedy bandit arms");
}

void EpsilonGreedyBandit::UpdateArm(const uint64_t reward,
                                    const std::string& segment) const {
  std::string json = AdsClientHelper::GetInstance()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);

  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

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

  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  arm.pulls++;
  arm.value = arm.value + (1.0 / arm.pulls * (reward - arm.value));
  iter->second = arm;

  json = targeting::EpsilonGreedyBanditArms::ToJson(arms);

  AdsClientHelper::GetInstance()->SetStringPref(prefs::kEpsilonGreedyBanditArms,
                                                json);

  BLOG(1,
       "Epsilon greedy bandit arm was updated for " << segment << " segment");
}

}  // namespace processor
}  // namespace ads
