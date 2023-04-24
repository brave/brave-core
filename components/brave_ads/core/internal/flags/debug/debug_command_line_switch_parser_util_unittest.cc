/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/command_line_switch_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  bool expected_should_debug;
} const kTests[] = {{{kRewardsSwitch, "debug=true"}, true},
                    {{kRewardsSwitch, "debug=1"}, true},
                    {{kRewardsSwitch, "debug=false"}, false},
                    {{kRewardsSwitch, "debug=foobar"}, false},
                    {{}, false}};

}  // namespace

class BraveAdsDebugCommandLineSwitchParserUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BraveAdsDebugCommandLineSwitchParserUtilTest,
       ParseDebugCommandLineSwitch) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_should_debug,
            GlobalState::GetInstance()->Flags().should_debug);
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string expected_should_debug =
      test_param.param.expected_should_debug ? "ShouldDebug" : "ShouldNotDebug";

  const std::string sanitized_command_line_switch =
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

  return base::ReplaceStringPlaceholders(
      "$1For$2", {expected_should_debug, sanitized_command_line_switch},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDebugCommandLineSwitchParserUtilTest,
                         testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
