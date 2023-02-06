/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/common/unittest/command_line_switch_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/environment/environment_types_unittest_util.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/flags/flag_manager_constants.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  EnvironmentType expected_environment_type;
} const kTestCases[] = {
    {/*command_line_switch*/ {kRewardsSwitch, "staging=true"},
     /*expected_environment_type*/ EnvironmentType::kStaging},
    {/*command_line_switch*/ {kRewardsSwitch, "staging=1"},
     /*expected_environment_type*/ EnvironmentType::kStaging},
    {/*command_line_switch*/ {kRewardsSwitch, "staging=false"},
     /*expected_environment_type*/ EnvironmentType::kProduction},
    {/*command_line_switch*/ {kRewardsSwitch, "staging=foobar"},
     /*expected_environment_type*/ EnvironmentType::kProduction},
    {/*command_line_switch*/ {},
     /*expected_environment_type*/ kDefaultEnvironmentType}};

}  // namespace

class BatAdsEnvironmentCommandLineSwitchParserUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BatAdsEnvironmentCommandLineSwitchParserUtilTest,
       ParseEnvironmentCommandLineSwitch) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_environment_type,
            FlagManager::GetInstance()->GetEnvironmentType());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string expected_environment_type =
      EnvironmentTypeEnumToString(test_param.param.expected_environment_type);

  const std::string sanitized_command_line_switch =
      SanitizeCommandLineSwitch(test_param.param.command_line_switch);

  return base::StringPrintf("%sEnvironmentFor%s",
                            expected_environment_type.c_str(),
                            sanitized_command_line_switch.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsEnvironmentCommandLineSwitchParserUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace ads
