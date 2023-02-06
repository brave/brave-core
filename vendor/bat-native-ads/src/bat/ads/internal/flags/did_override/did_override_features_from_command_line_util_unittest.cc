/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override/did_override_features_from_command_line_util.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base_switches.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_features.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/common/unittest/command_line_switch_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/conversions/conversions_features.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ads_features.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ads_features.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification_features.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kFooBarSwitch[] = "foobar";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_from_command_line;
} const kTestCases[] = {
    {/*command_line_switch*/ {kFooBarSwitch, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {switches::kEnableFeatures, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {switches::kEnableFeatures, "FooBar"},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              features::kAdRewards.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              features::kServing.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              resource::features::kAntiTargeting.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              features::kConversions.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              features::kEligibleAds.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              targeting::features::kEpsilonGreedyBandit.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              exclusion_rules::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              inline_content_ads::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              new_tab_page_ads::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              permission_rules::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              targeting::features::kPurchaseIntent.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              targeting::features::kTextClassification.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {switches::kEnableFeatures,
                              user_activity::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         switches::kEnableFeatures,
         base::JoinString(
             {"Foo", user_activity::features::kFeature.name, "Bar"},
             ",")},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              "FooBar"},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              features::kAdRewards.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              features::kServing.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              resource::features::kAntiTargeting.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              features::kConversions.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              features::kEligibleAds.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::features::kEpsilonGreedyBandit.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              exclusion_rules::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              inline_content_ads::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              new_tab_page_ads::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              permission_rules::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::features::kPurchaseIntent.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::features::kTextClassification.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              user_activity::features::kFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         variations::switches::kForceFieldTrialParams,
         base::JoinString(
             {"Foo", user_activity::features::kFeature.name, "Bar"},
             ",")},
     /*expected_did_override_from_command_line*/ true}};

}  // namespace

class BatAdsDidOverrideFeaturesFromCommandLineUtilTest
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

TEST_P(BatAdsDidOverrideFeaturesFromCommandLineUtilTest,
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
                         BatAdsDidOverrideFeaturesFromCommandLineUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace ads
