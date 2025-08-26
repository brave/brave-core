/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/flags/flags_util.h"

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
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
  bool should_debug = false;
  bool did_override_command_line_switches = false;
  mojom::EnvironmentType environment_type = kDefaultEnvironmentType;
  base::flat_map<std::string, bool> ads_uuids;
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const ParamInfo kTests[] = {
    // Should debug.
    {.command_line_switches = {{"rewards", "debug=true"}},
     .should_debug = true},

    // Should not debug.
    {.command_line_switches = {{"rewards", "debug=false"}},
     .should_debug = false},

    // Override variations command-line switches.
    {.command_line_switches = {{variations::switches::kFakeVariationsChannel,
                                "foobar"}},
     .did_override_command_line_switches = true},

    // Do not override variations command-line switches.
    {.command_line_switches = {{variations::switches::kFakeVariationsChannel,
                                ""}},
     .did_override_command_line_switches = false},

    // Force staging environment from command-line switch.
    {.command_line_switches = {{"rewards", "staging=true"}},
     .environment_type = mojom::EnvironmentType::kStaging},

    // Force production environment from command-line switch.
    {.command_line_switches = {{"rewards", "staging=false"}},
     .environment_type = mojom::EnvironmentType::kProduction},

    // Use default environment.
    {.command_line_switches = {}, .environment_type = kDefaultEnvironmentType},

    // Ads UUIDs
    {.command_line_switches =
         {{"ads",
           R"(uuids=fd955b44-5b46-4359-a074-3bc700cb86bf,7bc35504-c891-4b80-afac-20c655a5566e)"}},
     .ads_uuids = {{"fd955b44-5b46-4359-a074-3bc700cb86bf", true},
                   {"7bc35504-c891-4b80-afac-20c655a5566e", true}}},
};

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
  EXPECT_EQ(GetParam().ads_uuids, mojom_flags->ads_uuids);
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
