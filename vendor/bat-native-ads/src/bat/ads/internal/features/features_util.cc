/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features_util.h"

#include <string>

#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification_features.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"

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

  BLOG(1, "Text classification feature is "
              << GetStatus(targeting::features::IsTextClassificationEnabled()));

  BLOG(1, "Epsilon greedy bandit feature is " << GetStatus(
              targeting::features::IsEpsilonGreedyBanditEnabled()));

  BLOG(1, "Purchase intent feature is "
              << GetStatus(targeting::features::IsPurchaseIntentEnabled()));

  BLOG(1, "Permission rule feature is "
              << GetStatus(permission_rules::features::IsEnabled()));

  BLOG(1, "Exclusion rule feature is "
              << GetStatus(exclusion_rules::features::IsEnabled()));

  BLOG(1, "User activity feature is "
              << GetStatus(user_activity::features::IsEnabled()));
}

}  // namespace ads
