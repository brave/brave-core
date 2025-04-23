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
  bool should_debug;
  bool did_override_command_line_switches;
  mojom::EnvironmentType environment_type;
} const kTests[] = {
    // Should debug.
    {.command_line_switches = {{"rewards", "debug=true"}},
     .should_debug = true,
     .did_override_command_line_switches = false,
     .environment_type = kDefaultEnvironmentType},

    // Should not debug.
    {.command_line_switches = {{"rewards", "debug=false"}},
     .should_debug = false,
     .did_override_command_line_switches = false,
     .environment_type = kDefaultEnvironmentType},

    // Override variations command-line switches.
    {.command_line_switches = {{variations::switches::kFakeVariationsChannel,
                                "foobar"}},
     .should_debug = false,
     .did_override_command_line_switches = true,
     .environment_type = kDefaultEnvironmentType},

    // Do not override variations command-line switches.
    {.command_line_switches = {{variations::switches::kFakeVariationsChannel,
                                ""}},
     .should_debug = false,
     .did_override_command_line_switches = false,
     .environment_type = kDefaultEnvironmentType},

    // Force staging environment from command-line switch.
    {.command_line_switches = {{"rewards", "staging=true"}},
     .should_debug = false,
     .did_override_command_line_switches = false,
     .environment_type = mojom::EnvironmentType::kStaging},

    // Force production environment from command-line switch.
    {.command_line_switches = {{"rewards", "staging=false"}},
     .should_debug = false,
     .did_override_command_line_switches = false,
     .environment_type = mojom::EnvironmentType::kProduction},

    // Use default environment.
    {.command_line_switches = {},
     .should_debug = false,
     .did_override_command_line_switches = false,
     .environment_type = kDefaultEnvironmentType}};

}  // namespace

class BraveAdsFlagsUtilTest : public test::TestBase,
                              public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches(GetParam().command_line_switches);
  }
};

TEST_P(BraveAdsFlagsUtilTest, BuildFlags) {
  // Act
  const mojom::FlagsPtr mojom_flags = BuildFlags();

  // Assert
  EXPECT_EQ(GetParam().should_debug, mojom_flags->should_debug);
  EXPECT_EQ(GetParam().did_override_command_line_switches,
            mojom_flags->did_override_from_command_line);
  EXPECT_EQ(GetParam().environment_type, mojom_flags->environment_type);
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  // Environment
  const std::string environment_type =
      test::ToString(test_param.param.environment_type);

  // When
  std::vector<std::string> flags;

  if (test_param.param.should_debug) {
    flags.emplace_back("ShouldDebug");
  }

  if (test_param.param.did_override_command_line_switches) {
    flags.emplace_back("DidOverride");
  }

  std::string when;
  if (!flags.empty()) {
    when = "When" + base::JoinString(flags, "And");
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
