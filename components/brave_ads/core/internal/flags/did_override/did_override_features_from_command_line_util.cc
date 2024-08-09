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
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_feature.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_feature.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_inline_content_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_new_tab_page_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_notification_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

namespace brave_ads {

namespace {

const base::Feature* const kFeatures[] = {
    &kAccountStatementFeature,
    &kAccountTokensFeature,
    &kSiteVisitFeature,
    &kAntiTargetingFeature,
    &kCatalogFeature,
    &kConfirmationsFeature,
    &kConversionsFeature,
    &kCreativeInlineContentAdModelBasedPredictorFeature,
    &kCreativeNewTabPageAdModelBasedPredictorFeature,
    &kCreativeNotificationAdModelBasedPredictorFeature,
    &kEligibleAdFeature,
    &kExclusionRulesFeature,
    &kAdHistoryFeature,
    &kInlineContentAdFeature,
    &kInlineContentAdServingFeature,
    &kIssuersFeature,
    &kNewTabPageAdFeature,
    &kNewTabPageAdServingFeature,
    &kNotificationAdFeature,
    &kNotificationAdServingFeature,
    &kPermissionRulesFeature,
    &kPromotedContentAdFeature,
    &kPurchaseIntentFeature,
    &kRedeemPaymentTokensFeature,
    &kRemindersFeature,
    &kSearchResultAdFeature,
    &kSubdivisionFeature,
    &kTextClassificationFeature,
    &kUserActivityFeature,
    &kUserIdleDetectionFeature};

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
  const auto features = ParseCommandLineSwitches();
  return base::ranges::any_of(kFeatures, [&features](
                                             const auto* const feature) {
    CHECK(feature);

    return base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
               feature->name) ||
           features.contains(feature->name);
  });
}

}  // namespace brave_ads
