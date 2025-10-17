/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switch_values_util.h"

#include <string>

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "components/variations/variations_switches.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  test::CommandLineSwitchInfo command_line_switch;
  bool did_override_command_line_switch = false;
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const ParamInfo kTests[] = {
    {.command_line_switch = {"foobar", ""},
     .did_override_command_line_switch = false},
    {.command_line_switch = {variations::switches::kFakeVariationsChannel, ""},
     .did_override_command_line_switch = false},
    {.command_line_switch = {variations::switches::kFakeVariationsChannel,
                             "FooBar"},
     .did_override_command_line_switch = true},
    {.command_line_switch = {variations::switches::kVariationsOverrideCountry,
                             ""},
     .did_override_command_line_switch = false},
    {.command_line_switch = {variations::switches::kVariationsOverrideCountry,
                             "FooBar"},
     .did_override_command_line_switch = true}};

}  // namespace

class BraveAdsDidOverrideCommandLineSwitchValuesUtilTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches({GetParam().command_line_switch});

    scoped_feature_list_.InitFromCommandLine(
        /*enable_features=*/GetParam().command_line_switch.value,
        /*disable_features=*/{});
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(BraveAdsDidOverrideCommandLineSwitchValuesUtilTest,
       DidOverrideCommandLineSwitchValues) {
  // Act & Assert
  EXPECT_EQ(GetParam().did_override_command_line_switch,
            DidOverrideCommandLineSwitchValues());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string did_override_command_line_switch =
      test_param.param.did_override_command_line_switch ? "DidOverride"
                                                        : "DidNotOverride";

  const std::string sanitized_command_line_switch =
      test::ToString(test_param.param.command_line_switch);

  return base::ReplaceStringPlaceholders(
      "Set$1CommandLineSwitchValuesFor$2",
      {did_override_command_line_switch, sanitized_command_line_switch},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDidOverrideCommandLineSwitchValuesUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
