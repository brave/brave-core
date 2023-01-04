/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/conversions/conversions_features.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ads_features.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ads_features.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification_features.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"

namespace ads {

namespace {

const base::Feature* const kFeatures[] = {
    &exclusion_rules::features::kFeature,
    &features::kAdRewards,
    &features::kConversions,
    &features::kEligibleAds,
    &features::kServing,
    &inline_content_ads::features::kFeature,
    &new_tab_page_ads::features::kFeature,
    &permission_rules::features::kFeature,
    &resource::features::kAntiTargeting,
    &targeting::features::kEpsilonGreedyBandit,
    &targeting::features::kPurchaseIntent,
    &targeting::features::kTextClassification,
    &user_activity::features::kFeature};

}  // namespace

bool DidOverrideFeaturesFromCommandLine() {
  return base::ranges::any_of(kFeatures, [](const auto* feature) {
    DCHECK(feature);

    return base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
        feature->name);
  });
}

}  // namespace ads
