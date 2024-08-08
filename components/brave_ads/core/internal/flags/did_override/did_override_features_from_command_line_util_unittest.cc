/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base_switches.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
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
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_feature.h"
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
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kFooBarSwitch[] = "foobar";

struct ParamInfo final {
  test::CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_from_command_line;
} const kTests[] = {
    {{kFooBarSwitch, {}}, false},

    // switches::kEnableFeatures:
    {{switches::kEnableFeatures, {}}, false},
    {{switches::kEnableFeatures, "FooBar"}, false},
    {{switches::kEnableFeatures,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{switches::kEnableFeatures,
      base::StrCat({kAntiTargetingFeature.name, ":param/value"})},
     true},
    {{switches::kEnableFeatures,
      base::StrCat({kTextClassificationFeature.name,
                    "<TrialName.GroupName:param/value"})},
     true},
    {{switches::kEnableFeatures, kAccountStatementFeature.name}, true},
    {{switches::kEnableFeatures, kAccountTokensFeature.name}, true},
    {{switches::kEnableFeatures, kSiteVisitFeature.name}, true},
    {{switches::kEnableFeatures, kAntiTargetingFeature.name}, true},
    {{switches::kEnableFeatures, kCatalogFeature.name}, true},
    {{switches::kEnableFeatures, kConfirmationsFeature.name}, true},
    {{switches::kEnableFeatures, kConversionsFeature.name}, true},
    {{switches::kEnableFeatures,
      kCreativeInlineContentAdModelBasedPredictorFeature.name},
     true},
    {{switches::kEnableFeatures,
      kCreativeNewTabPageAdModelBasedPredictorFeature.name},
     true},
    {{switches::kEnableFeatures,
      kCreativeNotificationAdModelBasedPredictorFeature.name},
     true},
    {{switches::kEnableFeatures, kEligibleAdFeature.name}, true},
    {{switches::kEnableFeatures, kExclusionRulesFeature.name}, true},
    {{switches::kEnableFeatures, kAdHistoryFeature.name}, true},
    {{switches::kEnableFeatures, kInlineContentAdFeature.name}, true},
    {{switches::kEnableFeatures, kInlineContentAdServingFeature.name}, true},
    {{switches::kEnableFeatures, kIssuersFeature.name}, true},
    {{switches::kEnableFeatures, kNewTabPageAdFeature.name}, true},
    {{switches::kEnableFeatures, kNewTabPageAdServingFeature.name}, true},
    {{switches::kEnableFeatures, kNotificationAdFeature.name}, true},
    {{switches::kEnableFeatures, kNotificationAdServingFeature.name}, true},
    {{switches::kEnableFeatures, kPermissionRulesFeature.name}, true},
    {{switches::kEnableFeatures, kPromotedContentAdFeature.name}, true},
    {{switches::kEnableFeatures, kPurchaseIntentFeature.name}, true},
    {{switches::kEnableFeatures, kRedeemPaymentTokensFeature.name}, true},
    {{switches::kEnableFeatures, kRemindersFeature.name}, true},
    {{switches::kEnableFeatures, kSearchResultAdFeature.name}, true},
    {{switches::kEnableFeatures, kSubdivisionFeature.name}, true},
    {{switches::kEnableFeatures, kTextClassificationFeature.name}, true},
    {{switches::kEnableFeatures, kUserActivityFeature.name}, true},
    {{switches::kEnableFeatures, kUserIdleDetectionFeature.name}, true},

    // variations::switches::kForceFieldTrialParams:
    {{variations::switches::kForceFieldTrialParams, {}}, false},
    {{variations::switches::kForceFieldTrialParams, "FooBar"}, false},
    {{variations::switches::kForceFieldTrialParams,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kAccountStatementFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAccountTokensFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kSiteVisitFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAntiTargetingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kCatalogFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kConfirmationsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kConversionsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kCreativeInlineContentAdModelBasedPredictorFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kCreativeNewTabPageAdModelBasedPredictorFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kCreativeNotificationAdModelBasedPredictorFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kEligibleAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kExclusionRulesFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAdHistoryFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kInlineContentAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kInlineContentAdServingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kIssuersFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kNewTabPageAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kNewTabPageAdServingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kNotificationAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kNotificationAdServingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kPermissionRulesFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kPromotedContentAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kPurchaseIntentFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kRedeemPaymentTokensFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kRemindersFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kSearchResultAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kSubdivisionFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kTextClassificationFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kUserActivityFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kUserIdleDetectionFeature.name},
     true}};

}  // namespace

class BraveAdsDidOverrideFeaturesFromCommandLineUtilTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    const ParamInfo param = GetParam();

    test::AppendCommandLineSwitches({param.command_line_switch});

    std::unique_ptr<base::FeatureList> feature_list =
        std::make_unique<base::FeatureList>();
    feature_list->InitFromCommandLine(param.command_line_switch.value, {});
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
       DidOverrideFeaturesFromCommandLine) {
  // Act & Assert
  EXPECT_EQ(GetParam().expected_did_override_from_command_line,
            DidOverrideFeaturesFromCommandLine());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string expected_did_override_from_command_line =
      test_param.param.expected_did_override_from_command_line
          ? "DidOverride"
          : "DidNotOverride";

  const std::string sanitized_command_line_switch =
      test::ToString(test_param.param.command_line_switch);

  return base::ReplaceStringPlaceholders(
      "Set$1FeaturesFor$2",
      {expected_did_override_from_command_line, sanitized_command_line_switch},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
