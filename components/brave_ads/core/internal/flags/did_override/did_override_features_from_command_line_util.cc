/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/account/statement/ad_rewards_features.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/promoted_content_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/search_result_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_features.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_features.h"

namespace brave_ads {

namespace {

const base::Feature* const kFeatures[] = {
    &exclusion_rules::features::kFeature,
    &features::kAdRewards,
    &features::kConversions,
    &features::kEligibleAds,
    &inline_content_ads::features::kFeature,
    &inline_content_ads::features::kServing,
    &new_tab_page_ads::features::kFeature,
    &new_tab_page_ads::features::kServing,
    &notification_ads::features::kFeature,
    &notification_ads::features::kServing,
    &permission_rules::features::kFeature,
    &promoted_content_ads::features::kServing,
    &resource::features::kAntiTargeting,
    &search_result_ads::features::kServing,
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

}  // namespace brave_ads
