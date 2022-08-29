/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/base/unittest/command_line_switch_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/flags/flag_manager_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_should_debug;
} kTests[] = {{/*command_line_switch*/ {kRewardsSwitch, "debug=true"},
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
  BatAdsDebugCommandLineSwitchParserUtilTest() = default;

  ~BatAdsDebugCommandLineSwitchParserUtilTest() override = default;

  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BatAdsDebugCommandLineSwitchParserUtilTest,
       ParseDebugCommandLineSwitch) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_should_debug, ShouldDebug());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  const std::string expected_should_debug =
      test_param.param.expected_should_debug ? "ShouldDebug" : "ShouldNotDebug";

  const std::string sanitized_command_line_switch =
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

  return base::StringPrintf("%sFor%s", expected_should_debug.c_str(),
                            sanitized_command_line_switch.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsDebugCommandLineSwitchParserUtilTest,
                         testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace ads
