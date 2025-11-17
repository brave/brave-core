/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include <string>

#include "base/base_switches.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_feature.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_feature.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_feature.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_new_tab_page_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_notification_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  test::CommandLineSwitchInfo command_line_switch;
  bool did_override_from_command_line = false;
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const ParamInfo kTests[] = {
    {.command_line_switch = {"foobar", ""},
     .did_override_from_command_line = false},

    // switches::kEnableFeatures.
    {.command_line_switch = {switches::kEnableFeatures, ""},
     .did_override_from_command_line = false},
    {.command_line_switch = {switches::kEnableFeatures, "FooBar"},
     .did_override_from_command_line = false},
    {.command_line_switch = {switches::kEnableFeatures,
                             base::JoinString(
                                 {"Foo", kUserActivityFeature.name, "Bar"},
                                 ",")},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             std::string(kAntiTargetingFeature.name) +
                                 ":param/value"},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             std::string(kTextClassificationFeature.name) +
                                 "<TrialName.GroupName:param/value"},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kAccountStatementFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kAccountTokensFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures, kSiteVisitFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kAntiTargetingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures, kCatalogFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kConfirmationsFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kConversionsFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch =
         {switches::kEnableFeatures,
          kCreativeNewTabPageAdModelBasedPredictorFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch =
         {switches::kEnableFeatures,
          kCreativeNotificationAdModelBasedPredictorFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kEligibleAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kExclusionRulesFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures, kAdHistoryFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures, kIssuersFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kNewTabPageAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kNewTabPageAdServingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kNotificationAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kNotificationAdServingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kPermissionRulesFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kPurchaseIntentFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kRedeemPaymentTokensFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures, kRemindersFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kSearchResultAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kSubdivisionFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kTextClassificationFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kUserActivityFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {switches::kEnableFeatures,
                             kUserIdleDetectionFeature.name},
     .did_override_from_command_line = true},

    // variations::switches::kForceFieldTrialParams.
    {.command_line_switch = {variations::switches::kForceFieldTrialParams, ""},
     .did_override_from_command_line = false},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             "FooBar"},
     .did_override_from_command_line = false},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             base::JoinString(
                                 {"Foo", kUserActivityFeature.name, "Bar"},
                                 ",")},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kAccountStatementFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kAccountTokensFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kSiteVisitFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kAntiTargetingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kCatalogFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kConfirmationsFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kConversionsFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch =
         {variations::switches::kForceFieldTrialParams,
          kCreativeNewTabPageAdModelBasedPredictorFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch =
         {variations::switches::kForceFieldTrialParams,
          kCreativeNotificationAdModelBasedPredictorFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kEligibleAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kExclusionRulesFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kAdHistoryFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kIssuersFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kNewTabPageAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kNewTabPageAdServingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kNotificationAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kNotificationAdServingFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kPermissionRulesFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kPurchaseIntentFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kRedeemPaymentTokensFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kRemindersFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kSearchResultAdFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kSubdivisionFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kTextClassificationFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kUserActivityFeature.name},
     .did_override_from_command_line = true},
    {.command_line_switch = {variations::switches::kForceFieldTrialParams,
                             kUserIdleDetectionFeature.name},
     .did_override_from_command_line = true}};

}  // namespace

class BraveAdsDidOverrideFeaturesFromCommandLineUtilTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches({GetParam().command_line_switch});

    scoped_feature_list_.InitFromCommandLine(
        /*enable_features=*/GetParam().command_line_switch.value,
        /*disable_features=*/{});
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
       DidOverrideFeaturesFromCommandLine) {
  // Act & Assert
  EXPECT_EQ(GetParam().did_override_from_command_line,
            DidOverrideFeaturesFromCommandLine());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string did_override_from_command_line =
      test_param.param.did_override_from_command_line ? "DidOverride"
                                                      : "DidNotOverride";

  const std::string sanitized_command_line_switch =
      test::ToString(test_param.param.command_line_switch);

  return base::ReplaceStringPlaceholders(
      "Set$1FeaturesFor$2",
      {did_override_from_command_line, sanitized_command_line_switch}, nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
