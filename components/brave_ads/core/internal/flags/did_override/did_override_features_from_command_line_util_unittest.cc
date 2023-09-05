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
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_feature.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"
#include "brave/components/brave_ads/core/public/feature/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/feature/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/public/feature/user_attention_feature.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kFooBarSwitch[] = "foobar";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_from_command_line;
} const kTests[] = {
    {{kFooBarSwitch, {}}, false},
    {{::switches::kEnableFeatures, "FooBar"}, false},
    {{::switches::kEnableFeatures,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{::switches::kEnableFeatures,
      base::StrCat({kAntiTargetingFeature.name, ":param/value"})},
     true},
    {{::switches::kEnableFeatures,
      base::StrCat(
          {kEpsilonGreedyBanditFeatures.name, "<TrialName:param/value"})},
     true},
    {{::switches::kEnableFeatures,
      base::StrCat({kTextClassificationFeature.name,
                    "<TrialName.GroupName:param/value"})},
     true},
    {{::switches::kEnableFeatures, kAccountStatementFeature.name}, true},
    {{::switches::kEnableFeatures, kAccountTokensFeature.name}, true},
    {{::switches::kEnableFeatures, kAntiTargetingFeature.name}, true},
    {{::switches::kEnableFeatures, kConversionsFeature.name}, true},
    {{::switches::kEnableFeatures, kEligibleAdFeature.name}, true},
    {{::switches::kEnableFeatures, kEpsilonGreedyBanditFeatures.name}, true},
    {{::switches::kEnableFeatures, kExclusionRulesFeature.name}, true},
    {{::switches::kEnableFeatures, kInlineContentAdFeature.name}, true},
    {{::switches::kEnableFeatures, kNewTabPageAdFeature.name}, true},
    {{::switches::kEnableFeatures, kNotificationAdFeature.name}, true},
    {{::switches::kEnableFeatures, kPermissionRulesFeature.name}, true},
    {{::switches::kEnableFeatures, kPromotedContentAdFeature.name}, true},
    {{::switches::kEnableFeatures, kPurchaseIntentFeature.name}, true},
    {{::switches::kEnableFeatures, kSearchResultAdFeature.name}, true},
    {{::switches::kEnableFeatures, kTextClassificationFeature.name}, true},
    {{::switches::kEnableFeatures, kUserActivityFeature.name}, true},
    {{::switches::kEnableFeatures, kUserAttentionFeature.name}, true},
    {{::switches::kEnableFeatures, {}}, false},
    {{variations::switches::kForceFieldTrialParams, "FooBar"}, false},
    {{variations::switches::kForceFieldTrialParams,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kAccountStatementFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAccountTokensFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAntiTargetingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kConversionsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kEligibleAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kEpsilonGreedyBanditFeatures.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kExclusionRulesFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kInlineContentAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kNewTabPageAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kNotificationAdFeature.name},
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
      kSearchResultAdFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kTextClassificationFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kUserActivityFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kUserAttentionFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, {}}, false}};

}  // namespace

class BraveAdsDidOverrideFeaturesFromCommandLineUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    const ParamInfo param = GetParam();

    AppendCommandLineSwitches({param.command_line_switch});

    std::unique_ptr<base::FeatureList> feature_list =
        std::make_unique<base::FeatureList>();
    feature_list->InitializeFromCommandLine(param.command_line_switch.value,
                                            {});
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
       DidOverrideFeaturesFromCommandLine) {
  // Arrange

  // Act

  // Assert
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
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

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
