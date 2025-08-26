/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/ads_uuids/ads_uuids_command_line_switch_parser_util.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  test::CommandLineSwitchInfo command_line_switch;
  base::flat_map<std::string, bool> ads_uuids;
};

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const ParamInfo kTests[] = {
    {.command_line_switch = {"ads",
                             "uuids=52ee5e5a-08ae-4295-9bfe-a2d802144c86"},
     .ads_uuids = {{"52ee5e5a-08ae-4295-9bfe-a2d802144c86", true}}},
    {.command_line_switch =
         {"ads",
          R"(uuids=52ee5e5a-08ae-4295-9bfe-a2d802144c86,123e4567-e89b-12d3-a456-426614174000)"},
     .ads_uuids = {{"52ee5e5a-08ae-4295-9bfe-a2d802144c86", true},
                   {"123e4567-e89b-12d3-a456-426614174000", true}}},
    {.command_line_switch = {"ads", "uuids="}, .ads_uuids = {}},
    {.command_line_switch = {"ads", ""}, .ads_uuids = {}},
    {.command_line_switch = {}, .ads_uuids = {}}};

}  // namespace

class BraveAdsUuidsCommandLineSwitchParserUtilTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::AppendCommandLineSwitches({GetParam().command_line_switch});
  }
};

TEST_P(BraveAdsUuidsCommandLineSwitchParserUtilTest,
       ParseAdsUuidsCommandLineSwitch) {
  // Act & Assert
  ASSERT_TRUE(GlobalState::HasInstance());
  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_generate_random_uuids=*/false);
  EXPECT_EQ(GetParam().ads_uuids, ParseAdsUuidsCommandLineSwitch());
}

std::string TestParamToString(
    const ::testing::TestParamInfo<ParamInfo>& test_param) {
  return test::ToString(test_param.param.command_line_switch);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsUuidsCommandLineSwitchParserUtilTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
