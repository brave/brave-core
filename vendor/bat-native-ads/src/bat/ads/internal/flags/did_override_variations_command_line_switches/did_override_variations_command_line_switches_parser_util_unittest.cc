/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override_variations_command_line_switches/did_override_variations_command_line_switches_parser_util.h"

#include <string>

#include "base/base_switches.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/base/unittest/command_line_switch_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/flags/flag_manager_util.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_did_override_command_line_switches;
};

constexpr char kFooBarSwitch[] = "foobar";

const ParamInfo kTests[] = {
    {{kFooBarSwitch, ""},
     /* expected_did_override_command_line_switches */ false},
    {{switches::kEnableFeatures, ""},
     /* expected_did_override_command_line_switches */ false},
    {{switches::kEnableFeatures, "FooBar"},
     /* expected_did_override_command_line_switches */ false},
    {{switches::kEnableFeatures, "AdRewards"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "AdServing"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "AntiTargeting"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "Conversions"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "EligibleAds"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "EpsilonGreedyBandit"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "FrequencyCapping"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "InlineContentAds"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "NewTabPageAds"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "PermissionRules"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "PurchaseIntent"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "TextClassification"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "UserActivity"},
     /* expected_did_override_command_line_switches */ true},
    {{switches::kEnableFeatures, "Foo,UserActivity,Bar"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kFakeVariationsChannel, ""},
     /* expected_did_override_command_line_switches */ false},
    {{variations::switches::kFakeVariationsChannel, "FooBar"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kVariationsOverrideCountry, ""},
     /* expected_did_override_command_line_switches */ false},
    {{variations::switches::kVariationsOverrideCountry, "FooBar"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, ""},
     /* expected_did_override_command_line_switches */ false},
    {{variations::switches::kForceFieldTrialParams, "FooBar"},
     /* expected_did_override_command_line_switches */ false},
    {{variations::switches::kForceFieldTrialParams, "AdRewards"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "AdServing"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "AntiTargeting"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "Conversions"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "EligibleAds"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "EpsilonGreedyBandit"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "FrequencyCapping"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "InlineContentAds"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "NewTabPageAds"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "PermissionRules"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "PurchaseIntent"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "TextClassification"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "UserActivity"},
     /* expected_did_override_command_line_switches */ true},
    {{variations::switches::kForceFieldTrialParams, "Foo,UserActivity,Bar"},
     /* expected_did_override_command_line_switches */ true}};

}  // namespace

class BatAdsDidOverrideVariationsCommandLineSwitchesParserUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  BatAdsDidOverrideVariationsCommandLineSwitchesParserUtilTest() = default;

  ~BatAdsDidOverrideVariationsCommandLineSwitchesParserUtilTest() override =
      default;

  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BatAdsDidOverrideVariationsCommandLineSwitchesParserUtilTest,
       ParseVariationsCommandLineSwitches) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_did_override_command_line_switches,
            DidOverrideVariationsCommandLineSwitches());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  const std::string expected_did_override_command_line_switches =
      test_param.param.expected_did_override_command_line_switches
          ? "DidOverride"
          : "DidNotOverride";

  const std::string sanitized_command_line_switch =
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

  return base::StringPrintf("Set%sVariationsFor%s",
                            expected_did_override_command_line_switches.c_str(),
                            sanitized_command_line_switch.c_str());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BatAdsDidOverrideVariationsCommandLineSwitchesParserUtilTest,
    testing::ValuesIn(kTests),
    TestParamToString);

}  // namespace ads
