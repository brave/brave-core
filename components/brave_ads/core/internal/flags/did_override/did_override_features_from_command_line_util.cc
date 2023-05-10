/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include <string>

#include "base/base_switches.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/common/notification_ad_feature.h"
#include "brave/components/brave_ads/common/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/internal/account/account_feature.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"

namespace brave_ads {

namespace {

const base::Feature* const kFeatures[] = {
    &kAccountFeature,
    &kAntiTargetingFeature,
    &kConversionsFeature,
    &kEligibleAdFeature,
    &kEpsilonGreedyBanditFeatures,
    &kExclusionRulesFeature,
    &kInlineContentAdFeature,
    &kNewTabPageAdFeature,
    &kNotificationAdFeature,
    &kPermissionRulesFeature,
    &kPromotedContentAdFeature,
    &kPurchaseIntentFeature,
    &kSearchResultAdFeature,
    &kShouldTriggerSearchResultAdEventsFeature,
    &kTextClassificationFeature,
    &kUserActivityFeature};

constexpr char kFeaturesSeparators[] = ",:<";

base::flat_set<std::string> ParseCommandLineSwitches() {
  const auto* const command_line = base::CommandLine::ForCurrentProcess();
  const std::string enabled_features_switch =
      command_line->GetSwitchValueASCII(::switches::kEnableFeatures);
  base::flat_set<std::string> enabled_features =
      base::SplitString(enabled_features_switch, kFeaturesSeparators,
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  const std::string disabled_features_switch =
      command_line->GetSwitchValueASCII(::switches::kDisableFeatures);
  base::flat_set<std::string> disabled_features =
      base::SplitString(disabled_features_switch, kFeaturesSeparators,
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  base::flat_set<std::string> features;
  base::ranges::set_union(enabled_features, disabled_features,
                          std::inserter(features, features.begin()));
  return features;
}

}  // namespace

bool DidOverrideFeaturesFromCommandLine() {
  const auto brave_ads_features = ParseCommandLineSwitches();
  return base::ranges::any_of(kFeatures, [&brave_ads_features](
                                             const auto* const feature) {
    CHECK(feature);

    return base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
               feature->name) ||
           brave_ads_features.contains(feature->name);
  });
}

}  // namespace brave_ads
