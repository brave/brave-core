/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features_util.h"

#include <string>

#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/features/frequency_capping_features.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/internal/serving/targeting/models/behavioral/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/serving/targeting/models/behavioral/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/serving/targeting/models/contextual/text_classification/text_classification_features.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_features.h"

namespace ads {

namespace {

std::string GetStatus(const bool status) {
  return status ? "enabled" : "disabled";
}

}  // namespace

void LogFeatures() {
  BLOG(1,
       "Ad rewards feature is " << GetStatus(features::IsAdRewardsEnabled()));

  BLOG(1, "Ad serving feature is " << GetStatus(features::IsServingEnabled()));

  BLOG(1, "Epsilon greedy bandit feature is "
              << GetStatus(features::IsEpsilonGreedyBanditEnabled()));

  BLOG(1, "Frequency capping feature is "
              << GetStatus(features::frequency_capping::IsEnabled()));

  BLOG(1, "Purchase intent feature is "
              << GetStatus(features::IsPurchaseIntentEnabled()));

  BLOG(1, "Text classification feature is "
              << GetStatus(features::IsTextClassificationEnabled()));

  BLOG(1, "User activity feature is "
              << GetStatus(features::user_activity::IsEnabled()));
}

}  // namespace ads
