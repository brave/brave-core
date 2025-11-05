/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types_test_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  test::CommandLineSwitchInfo command_line_switch;
  mojom::EnvironmentType environment_type = kDefaultEnvironmentType;
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const ParamInfo kTests[] = {
    {.command_line_switch = {"rewards", "staging=true"},
     .environment_type = mojom::EnvironmentType::kStaging},
    {.command_line_switch = {"rewards", "staging=1"},
     .environment_type = mojom::EnvironmentType::kStaging},
    {.command_line_switch = {"rewards", "staging=false"},
     .environment_type = mojom::EnvironmentType::kProduction},
    {.command_line_switch = {"rewards", "staging=foobar"},
     .environment_type = mojom::EnvironmentType::kProduction},
    {.command_line_switch = {}, .environment_type = kDefaultEnvironmentType}};

}  // namespace

class BraveAdsEnvironmentCommandLineSwitchParserUtilTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BraveAdsEnvironmentCommandLineSwitchParserUtilTest,
       ParseEnvironmentCommandLineSwitch) {
  // Act & Assert
  ASSERT_TRUE(GlobalState::HasInstance());
  EXPECT_EQ(GetParam().environment_type,
            GlobalState::GetInstance()->Flags().environment_type);
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string environment_type =
      test::ToString(test_param.param.environment_type);

  const std::string sanitized_command_line_switch =
      test::ToString(test_param.param.command_line_switch);

  return base::ReplaceStringPlaceholders(
      "$1EnvironmentFor$2", {environment_type, sanitized_command_line_switch},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsEnvironmentCommandLineSwitchParserUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
