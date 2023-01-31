/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/flag_manager.h"

#include <string>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/common/unittest/command_line_switch_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/environment/environment_types_unittest_util.h"
#include "bat/ads/internal/flags/flag_manager_constants.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kRewardsSwitch[] = "rewards";

struct ParamInfo final {
  bool should_force_staging_environment;
  CommandLineSwitchList command_line_switches;
  bool expected_should_debug;
  bool expected_did_override_command_line_switches;
  EnvironmentType expected_environment_type;
} const kTestCases[] = {
    // Should debug
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/ {{kRewardsSwitch, "debug=true"}},
     /*expected_should_debug*/ true,
     /*expected_did_override_command_line_switches*/ false,
     kDefaultEnvironmentType},

    // Should not debug
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/ {{kRewardsSwitch, "debug=false"}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     kDefaultEnvironmentType},

    // Override variations command-line switches
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/
     {{variations::switches::kFakeVariationsChannel, "foobar"}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ true,
     kDefaultEnvironmentType},

    // Do not override variations command-line switches
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/
     {{variations::switches::kFakeVariationsChannel, {}}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     kDefaultEnvironmentType},

    // Force staging environment from command-line switch
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/ {{kRewardsSwitch, "staging=true"}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     EnvironmentType::kStaging},

    // Force production environment from command-line switch
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/ {{kRewardsSwitch, "staging=false"}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     EnvironmentType::kProduction},

    // Force staging environment
    {/*should_force_staging_environment*/ true,
     /*command_line_switches*/ {{kRewardsSwitch, "staging=false"}},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     EnvironmentType::kStaging},

    // Use default environment
    {/*should_force_staging_environment*/ false,
     /*command_line_switches*/ {},
     /*expected_should_debug*/ false,
     /*expected_did_override_command_line_switches*/ false,
     kDefaultEnvironmentType}};

}  // namespace

class BatAdsFlagManagerTest : public UnitTestBase,
                              public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    const ParamInfo param = GetParam();

    AdsClientHelper::GetInstance()->SetBooleanPref(
        brave_rewards::prefs::kUseRewardsStagingServer,
        param.should_force_staging_environment);

    AppendCommandLineSwitches(param.command_line_switches);
  }
};

TEST_P(BatAdsFlagManagerTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(FlagManager::HasInstance());
}

TEST_P(BatAdsFlagManagerTest, Initialize) {
  // Arrange
  const ParamInfo param = GetParam();

  // Act

  // Assert
  EXPECT_EQ(param.expected_should_debug,
            FlagManager::GetInstance()->ShouldDebug());
  EXPECT_EQ(param.expected_did_override_command_line_switches,
            FlagManager::GetInstance()->DidOverrideFromCommandLine());
  EXPECT_EQ(param.expected_environment_type,
            FlagManager::GetInstance()->GetEnvironmentType());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  // Environment
  const std::string environment_type =
      EnvironmentTypeEnumToString(test_param.param.expected_environment_type);

  // When
  std::vector<std::string> flags;

  if (test_param.param.should_force_staging_environment) {
    flags.emplace_back("ShouldForceStagingEnvironment");
  }

  if (test_param.param.expected_should_debug) {
    flags.emplace_back("ShouldDebug");
  }

  if (test_param.param.expected_did_override_command_line_switches) {
    flags.emplace_back("DidOverride");
  }

  std::string when;
  if (!flags.empty()) {
    when = base::StrCat({"When", base::JoinString(flags, "And")});
  }

  // With
  std::string with;

  if (!test_param.param.command_line_switches.empty()) {
    std::vector<std::string> sanitized_command_line_switches;

    for (const auto& command_line_switch :
         test_param.param.command_line_switches) {
      if (command_line_switch.key.empty()) {
        continue;
      }

      sanitized_command_line_switches.push_back(
          SanitizeCommandLineSwitch(command_line_switch));
    }

    with = base::StrCat(
        {"With", base::JoinString(sanitized_command_line_switches, "And"),
         "CommandLineSwitches"});
  } else {
    with = "WithNoCommandLineSwitches";
  }

  return base::StringPrintf("ShouldUse%sEnvironment%s%s",
                            environment_type.c_str(), when.c_str(),
                            with.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsFlagManagerTest,
                         testing::ValuesIn(kTestCases),
                         TestParamToString);

}  // namespace ads
