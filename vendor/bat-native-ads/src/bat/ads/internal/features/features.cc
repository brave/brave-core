/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features.h"

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/features/ad_rewards/ad_rewards_features.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace features {

namespace {
const char kAdsTrialTag[] = "BraveAds";
}  // namespace

base::FieldTrial::ActiveGroups GetStudies() {
  base::FieldTrial::ActiveGroups studies;
  base::FieldTrialList::GetActiveFieldTrialGroups(&studies);

  base::FieldTrial::ActiveGroups filtered_studies;
  for (const auto& group : studies) {
    if (group.trial_name.find(kAdsTrialTag) != std::string::npos) {
      filtered_studies.push_back(group);
    }
  }
  return filtered_studies;
}

void Log() {
  const base::FieldTrial::ActiveGroups studies = GetStudies();
  if (studies.empty()) {
    BLOG(1, "No active studies");
  } else {
    for (const auto& study : studies) {
      BLOG(1, "Study " << study.trial_name << " is active (" << study.group_name
                       << ")");
    }
  }

  BLOG(1, "Text classification feature is "
              << (IsTextClassificationEnabled() ? "enabled" : "disabled"));

  BLOG(1, "Epsilon greedy bandit feature is "
              << (IsEpsilonGreedyBanditEnabled() ? "enabled" : "disabled"));

  BLOG(1, "Purchase intent feature is "
              << (IsPurchaseIntentEnabled() ? "enabled" : "disabled"));

  BLOG(1, "Ad rewards feature is "
              << (IsAdRewardsEnabled() ? "enabled" : "disabled"));

  BLOG(1, "Ad serving feature is "
              << (IsAdServingEnabled() ? "enabled" : "disabled"));

  BLOG(1, "User activity feature is "
              << (user_activity::IsEnabled() ? "enabled" : "disabled"));

  BLOG(1, "Frequency capping feature is "
              << (frequency_capping::IsEnabled() ? "enabled" : "disabled"));
}

}  // namespace features
}  // namespace ads
