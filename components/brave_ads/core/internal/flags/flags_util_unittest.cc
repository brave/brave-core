/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/flags/flags_util.h"

#include <string>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types_test_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  test::CommandLineSwitchList command_line_switches;
  bool expected_should_debug;
  bool expected_did_override_command_line_switches;
  mojom::EnvironmentType expected_environment_type;
} const kTests[] = {
    // Should debug
    {{{"rewards", "debug=true"}}, true, false, kDefaultEnvironmentType},

    // Should not debug
    {{{"rewards", "debug=false"}}, false, false, kDefaultEnvironmentType},

    // Override variations command-line switches
    {{{variations::switches::kFakeVariationsChannel, "foobar"}},
     false,
     true,
     kDefaultEnvironmentType},

    // Do not override variations command-line switches
    {{{variations::switches::kFakeVariationsChannel, {}}},
     false,
     false,
     kDefaultEnvironmentType},

    // Force staging environment from command-line switch
    {{{"rewards", "staging=true"}},
     false,
     false,
     mojom::EnvironmentType::kStaging},

    // Force production environment from command-line switch
    {{{"rewards", "staging=false"}},
     false,
     false,
     mojom::EnvironmentType::kProduction},

    // Use default environment
    {{}, false, false, kDefaultEnvironmentType}};

}  // namespace

class BraveAdsFlagsUtilTest : public test::TestBase,
                              public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches(GetParam().command_line_switches);
  }
};

TEST_P(BraveAdsFlagsUtilTest, BuildFlags) {
  // Arrange
  const ParamInfo param = GetParam();

  // Act
  const mojom::FlagsPtr mojom_flags = BuildFlags();

  // Assert
  EXPECT_EQ(param.expected_should_debug, mojom_flags->should_debug);
  EXPECT_EQ(param.expected_did_override_command_line_switches,
            mojom_flags->did_override_from_command_line);
  EXPECT_EQ(param.expected_environment_type, mojom_flags->environment_type);
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  // Environment
  const std::string environment_type =
      test::ToString(test_param.param.expected_environment_type);

  // When
  std::vector<std::string> flags;

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
          test::ToString(command_line_switch));
    }

    with = base::StrCat(
        {"With", base::JoinString(sanitized_command_line_switches, "And"),
         "CommandLineSwitches"});
  } else {
    with = "WithNoCommandLineSwitches";
  }

  return base::ReplaceStringPlaceholders(
      "ShouldUse$1Environment$2$3", {environment_type, when, with}, nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsFlagsUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
