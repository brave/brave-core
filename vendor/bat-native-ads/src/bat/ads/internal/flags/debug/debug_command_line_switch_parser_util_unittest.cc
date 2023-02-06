/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/common/unittest/command_line_switch_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/flags/flag_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_should_debug;
} const kTestCases[] = {
    {/*command_line_switch*/ {kRewardsSwitch, "debug=true"},
     /*expected_should_debug*/ true},
    {/*command_line_switch*/ {kRewardsSwitch, "debug=1"},
     /*expected_should_debug*/ true},
    {/*command_line_switch*/ {kRewardsSwitch, "debug=false"},
     /*expected_should_debug*/ false},
    {/*command_line_switch*/ {kRewardsSwitch, "debug=foobar"},
     /*expected_should_debug*/ false},
    {/*command_line_switch */ {}, /* expected_should_debug*/ false}};

}  // namespace

class BatAdsDebugCommandLineSwitchParserUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BatAdsDebugCommandLineSwitchParserUtilTest,
       ParseDebugCommandLineSwitch) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_should_debug,
            FlagManager::GetInstance()->ShouldDebug());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string expected_should_debug =
      test_param.param.expected_should_debug ? "ShouldDebug" : "ShouldNotDebug";

  const std::string sanitized_command_line_switch =
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

  return base::StringPrintf("%sFor%s", expected_should_debug.c_str(),
                            sanitized_command_line_switch.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsDebugCommandLineSwitchParserUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace ads
