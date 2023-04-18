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

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kFooBarSwitch[] = "foobar";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_from_command_line;
} const kTestCases[] = {
    {/*command_line_switch*/ {kFooBarSwitch, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {::switches::kEnableFeatures, "FooBar"},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {::switches::kEnableFeatures, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {
         ::switches::kEnableFeatures,
         base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kExclusionRulesFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kAccountFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kConversionsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kEligibleAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         ::switches::kEnableFeatures,
         features::kShouldTriggerSearchResultAdEvents.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              inline_content_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              new_tab_page_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              notification_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kPermissionRulesFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              promoted_content_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kAntiTargetingFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         ::switches::kEnableFeatures,
         base::StrCat({kAntiTargetingFeature.name, ":param/value"})},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              search_result_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              targeting::kEpsilonGreedyBanditFeatures.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         ::switches::kEnableFeatures,
         base::StrCat({targeting::kEpsilonGreedyBanditFeatures.name,
                       "<TrialName:param/value"})},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              targeting::kPurchaseIntentFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              targeting::kTextClassificationFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         ::switches::kEnableFeatures,
         base::StrCat({targeting::kTextClassificationFeature.name,
                       "<TrialName.GroupName:param/value"})},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {::switches::kEnableFeatures,
                              kUserActivityFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams, {}},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              "FooBar"},
     /*expected_did_override_from_command_line*/ false},
    {/*command_line_switch*/ {
         variations::switches::kForceFieldTrialParams,
         base::JoinString({"Foo", kUserActivityFeature.name, "Bar"}, ",")},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kExclusionRulesFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kAccountFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kConversionsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kEligibleAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {
         variations::switches::kForceFieldTrialParams,
         features::kShouldTriggerSearchResultAdEvents.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              inline_content_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              new_tab_page_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              notification_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kPermissionRulesFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              promoted_content_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kAntiTargetingFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              search_result_ads::kAdsFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::kEpsilonGreedyBanditFeatures.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::kPurchaseIntentFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              targeting::kTextClassificationFeature.name},
     /*expected_did_override_from_command_line*/ true},
    {/*command_line_switch*/ {variations::switches::kForceFieldTrialParams,
                              kUserActivityFeature.name},
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

  return base::ReplaceStringPlaceholders(
      "Set$1FeaturesFor$2",
      {expected_did_override_from_command_line, sanitized_command_line_switch},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsDidOverrideFeaturesFromCommandLineUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace brave_ads
