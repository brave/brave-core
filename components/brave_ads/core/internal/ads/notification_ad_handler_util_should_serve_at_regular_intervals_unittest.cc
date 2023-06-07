/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/notification_ad_handler_util.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  bool is_enabled;
  bool is_browser_active;
  bool can_show_while_browser_is_backgrounded;
  int ads_per_hour;
  bool should_serve_at_regular_intervals;
} constexpr kTests[] = {
    {/*is_enabled */ false, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ false, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ false,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ true},
    {/*is_enabled */ true, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ false, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ true},
    {/*is_enabled */ true, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 0,
     /*should_serve_at_regular_intervals*/ false},
    {/*is_enabled */ true, /* is_browser_active*/ true,
     /*can_show_while_browser_is_backgrounded */ true, /* ads_per_hour*/ 1,
     /*should_serve_at_regular_intervals*/ true}};

}  // namespace

class BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    const ParamInfo param = GetParam();

    SetDefaultBooleanPref(prefs::kEnabled, param.is_enabled);

    MockIsBrowserActive(ads_client_mock_, param.is_browser_active);

    MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
        ads_client_mock_, param.can_show_while_browser_is_backgrounded);

    SetDefaultInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                        param.ads_per_hour);
  }
};

TEST_P(BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest,
       NotificationAdHandler) {
  // Arrange
  const ParamInfo param = GetParam();

  // Act

  // Assert
  EXPECT_EQ(param.should_serve_at_regular_intervals,
            ShouldServeAtRegularIntervals());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  const std::string should_serve_at_regular_intervals =
      test_param.param.should_serve_at_regular_intervals
          ? "ShouldServeAtRegularIntervals"
          : "ShouldNotServeAtRegularIntervals";

  const std::string is_enabled = test_param.param.is_enabled
                                     ? "BravePrivateAdsAreEnabled"
                                     : "BravePrivateAdsAreDisabled";

  const std::string is_browser_active = test_param.param.is_browser_active
                                            ? "BrowserIsActive"
                                            : "BrowserIsInactive";

  const std::string can_show_while_browser_is_backgrounded =
      test_param.param.can_show_while_browser_is_backgrounded
          ? "CanShowWhileBrowserIsBackgrounded"
          : "CannotShowWhileBrowserIsBackgrounded";

  const std::string ads_per_hour =
      base::StringPrintf("%dAdsPerPerHour", test_param.param.ads_per_hour);

  return base::ReplaceStringPlaceholders(
      "$1If$2And$3And$4And$5",
      {should_serve_at_regular_intervals, is_enabled, is_browser_active,
       can_show_while_browser_is_backgrounded, ads_per_hour},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest,
    testing::ValuesIn(kTests),
    TestParamToString);

}  // namespace brave_ads
