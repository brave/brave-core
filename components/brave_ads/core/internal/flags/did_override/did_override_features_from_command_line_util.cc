/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include <string>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/core/ad_switches.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/account/account_features.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/search_result_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_features.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_features.h"

namespace brave_ads {

namespace {

const base::Feature* const kFeatures[] = {
    &features::kShouldTriggerSearchResultAdEvents,
    &inline_content_ads::kAdsFeature,
    &kAccountFeature,
    &kAntiTargetingFeature,
    &kConversionsFeature,
    &kEligibleAdsFeature,
    &kExclusionRulesFeature,
    &kPermissionRulesFeature,
    &kUserActivityFeature,
    &new_tab_page_ads::kAdsFeature,
    &notification_ads::kAdsFeature,
    &promoted_content_ads::kAdsFeature,
    &search_result_ads::kAdsFeature,
    &targeting::kEpsilonGreedyBanditFeatures,
    &targeting::kPurchaseIntentFeature,
    &targeting::kTextClassificationFeature};

constexpr char kFeaturesSeparators[] = ",:<";

base::flat_set<std::string> ParseCommandLineSwitches() {
  const auto* const command_line = base::CommandLine::ForCurrentProcess();
  const std::string features_switch =
      command_line->GetSwitchValueASCII(switches::kFeaturesSwitch);
  base::flat_set<std::string> features =
      base::SplitString(features_switch, kFeaturesSeparators,
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return features;
}

}  // namespace

bool DidOverrideFeaturesFromCommandLine() {
  const auto brave_ads_features = ParseCommandLineSwitches();
  return base::ranges::any_of(kFeatures, [&brave_ads_features](
                                             const auto* feature) {
    DCHECK(feature);

    return base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
               feature->name) ||
           brave_ads_features.contains(feature->name);
  });
}

}  // namespace brave_ads
