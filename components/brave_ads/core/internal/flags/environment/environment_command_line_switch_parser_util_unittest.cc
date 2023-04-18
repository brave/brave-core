/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/common/unittest/command_line_switch_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types_unittest_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  CommandLineSwitchInfo command_line_switch;
  mojom::EnvironmentType expected_environment_type;
} const kTestCases[] = {
    {{kRewardsSwitch, "staging=true"}, mojom::EnvironmentType::kStaging},
    {{kRewardsSwitch, "staging=1"}, mojom::EnvironmentType::kStaging},
    {{kRewardsSwitch, "staging=false"}, mojom::EnvironmentType::kProduction},
    {{kRewardsSwitch, "staging=foobar"}, mojom::EnvironmentType::kProduction},
    {{}, kDefaultEnvironmentType}};

}  // namespace

class BraveAdsEnvironmentCommandLineSwitchParserUtilTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BraveAdsEnvironmentCommandLineSwitchParserUtilTest,
       ParseEnvironmentCommandLineSwitch) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GetParam().expected_environment_type,
            GlobalState::GetInstance()->Flags().environment_type);
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
                         BraveAdsEnvironmentCommandLineSwitchParserUtilTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace brave_ads
