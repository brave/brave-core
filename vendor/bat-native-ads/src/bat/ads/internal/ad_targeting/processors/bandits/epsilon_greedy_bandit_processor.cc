/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/bandits/epsilon_greedy_bandit_processor.h"

#include <algorithm>
#include <vector>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit_arms.h"
#include "bat/ads/internal/ad_targeting/resources/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace ad_targeting {
namespace processor {

namespace {
const double kArmDefaultValue = 1.0;
const uint64_t kArmDefaultPulls = 0;
}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() {
  InitializeArms();
}

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

void EpsilonGreedyBandit::Process(
    const BanditFeedback& feedback) {
  // TODO(Moritz Haller): extract top level segment
  std::string segment = feedback.segment;
  // TODO(Moritz Haller): switch/case to make type safe
  if (feedback.ad_event_type == ads::AdNotificationEventType::kTimedOut ||
      feedback.ad_event_type == ads::AdNotificationEventType::kDismissed) {
    UpdateArm(/* reward */ 0, segment);
  } else if (feedback.ad_event_type == ads::AdNotificationEventType::kClicked) {
    UpdateArm(/* reward */ 1, segment);
  }

  BLOG(1, "Processed ad event");
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBandit::InitializeArms() const {
  std::string json = AdsClientHelper::Get()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);
  EpsilonGreedyBanditArmList arms = EpsilonGreedyBanditArms::FromJson(json);

  for (auto& arm : arms) {
    auto iter = std::find(resource::kSegments.begin(),
        resource::kSegments.end(), arm.segment);

    if (iter == resource::kSegments.end()) {
      // TODO(Moritz Haller): Delete w/ e.g. arms.erase(arm);
      BLOG(1, "DEBUG.1 ARM NOT IN kSegments - Deleted");
    }
  }

  // TODO(Moritz Haller): make sure resource doesn't  contain duplicates

  for (const auto& segment : resource::kSegments) {
    auto iter = std::find_if(arms.begin(), arms.end(),
        [&segment](const EpsilonGreedyBanditArmInfo& arm) {
      return arm.segment == segment;
    });

    if (iter == arms.end()) {
      EpsilonGreedyBanditArmInfo arm;
      arm.segment = segment;
      arm.value = kArmDefaultValue;
      arm.pulls = kArmDefaultPulls;
      arms.push_back(arm);
      BLOG(1, "DIDN'T FIND arm but added: " << arm.segment);
      continue;
    }

    if (!iter->IsValid()) {
      EpsilonGreedyBanditArmInfo new_arm;
      new_arm.segment = segment;
      new_arm.value = kArmDefaultValue;
      new_arm.pulls = kArmDefaultPulls;
      *iter = new_arm;
      BLOG(1, "FOUND INVALID arm and reset: " << new_arm.segment);
      continue;
    }

    BLOG(1, "FOUND valid arm: " << iter->segment);
  }

  std::string json_out = EpsilonGreedyBanditArms::ToJson(arms);
  AdsClientHelper::Get()->SetStringPref(
      prefs::kEpsilonGreedyBanditArms, json_out);

  BLOG(1, "Successfully initialized");
  BLOG(1, "Arms pref " << json_out);
}

void EpsilonGreedyBandit::UpdateArm(
    uint8_t reward,
    const std::string& segment) const {
  BLOG(1, "Update arms: pulls +1, reward +" << reward);
  std::string json = AdsClientHelper::Get()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);
  EpsilonGreedyBanditArmList arms = EpsilonGreedyBanditArms::FromJson(json);

  if (arms.empty()) {
    return;
  }

  // TODO(Moritz Haller): ad DCHECK to catch during dev if duplicate segments
  // exist - don't handle
  // TODO(Moritz Haller): maybe use set
  auto iter = std::find_if(arms.begin(), arms.end(),
      [&segment](const EpsilonGreedyBanditArmInfo& arm) {
    return arm.segment == segment;
  });

  // TODO(Moritz Haller): Add "untargeted" arm?
  if (iter == arms.end()) {
    BLOG(1, "Arm not found for segment " << segment);
    return;
  }

  iter->pulls = iter->pulls + 1;
  iter->value = iter->value + 1 / iter->pulls * (reward - iter->value);

  // TODO(Moritz Haller): make reward uint64

  std::string json_out = EpsilonGreedyBanditArms::ToJson(arms);
  BLOG(1, "DEBUG arms " << json_out);
  AdsClientHelper::Get()->SetStringPref(
      prefs::kEpsilonGreedyBanditArms, json_out);

  BLOG(1, "Arm updated for segment " << segment);
}

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads
