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
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/common/features.h"
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
#include "brave/components/brave_ads/core/internal/common/unittest/command_line_switch_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_features.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kFooBarSwitch[] = "foobar";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_from_command_line;
} const kTestCases[] = {
    {{kFooBarSwitch, {}}, false},
    {{::switches::kEnableFeatures, "FooBar"}, false},
    {{::switches::kEnableFeatures, {}}, false},
    {{::switches::kEnableFeatures,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{::switches::kEnableFeatures, kExclusionRulesFeature.name}, true},
    {{::switches::kEnableFeatures, kAccountFeature.name}, true},
    {{::switches::kEnableFeatures, kConversionsFeature.name}, true},
    {{::switches::kEnableFeatures, kEligibleAdsFeature.name}, true},
    {{::switches::kEnableFeatures,
      features::kShouldTriggerSearchResultAdEvents.name},
     true},
    {{::switches::kEnableFeatures, inline_content_ads::kAdsFeature.name}, true},
    {{::switches::kEnableFeatures, new_tab_page_ads::kAdsFeature.name}, true},
    {{::switches::kEnableFeatures, notification_ads::kAdsFeature.name}, true},
    {{::switches::kEnableFeatures, kPermissionRulesFeature.name}, true},
    {{::switches::kEnableFeatures, promoted_content_ads::kAdsFeature.name},
     true},
    {{::switches::kEnableFeatures, kAntiTargetingFeature.name}, true},
    {{::switches::kEnableFeatures,
      base::StrCat({kAntiTargetingFeature.name, ":param/value"})},
     true},
    {{::switches::kEnableFeatures, search_result_ads::kAdsFeature.name}, true},
    {{::switches::kEnableFeatures,
      targeting::kEpsilonGreedyBanditFeatures.name},
     true},
    {{::switches::kEnableFeatures,
      base::StrCat({targeting::kEpsilonGreedyBanditFeatures.name,
                    "<TrialName:param/value"})},
     true},
    {{::switches::kEnableFeatures, targeting::kPurchaseIntentFeature.name},
     true},
    {{::switches::kEnableFeatures, targeting::kTextClassificationFeature.name},
     true},
    {{::switches::kEnableFeatures,
      base::StrCat({targeting::kTextClassificationFeature.name,
                    "<TrialName.GroupName:param/value"})},
     true},
    {{::switches::kEnableFeatures, kUserActivityFeature.name}, true},
    {{variations::switches::kForceFieldTrialParams, {}}, false},
    {{variations::switches::kForceFieldTrialParams, "FooBar"}, false},
    {{variations::switches::kForceFieldTrialParams,
      base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kExclusionRulesFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAccountFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kConversionsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kEligibleAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      features::kShouldTriggerSearchResultAdEvents.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      inline_content_ads::kAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      new_tab_page_ads::kAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      notification_ads::kAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      kPermissionRulesFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      promoted_content_ads::kAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kAntiTargetingFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      search_result_ads::kAdsFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      targeting::kEpsilonGreedyBanditFeatures.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      targeting::kPurchaseIntentFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams,
      targeting::kTextClassificationFeature.name},
     true},
    {{variations::switches::kForceFieldTrialParams, kUserActivityFeature.name},
     true}};

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

  return base::StringPrintf("Set%sFeaturesFor%s",
                            expected_did_override_from_command_line.c_str(),
                            sanitized_command_line_switch.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDidOverrideFeaturesFromCommandLineUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace brave_ads
