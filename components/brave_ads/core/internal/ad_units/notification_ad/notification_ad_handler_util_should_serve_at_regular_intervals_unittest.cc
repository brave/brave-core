/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_handler_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  bool should_serve_at_regular_intervals = false;
  bool opted_in = false;
  bool should_browser_enter_foreground = false;
  bool can_show_while_browser_is_backgrounded = false;
} constexpr kTests[] = {{.should_serve_at_regular_intervals = false,
                         .opted_in = false,
                         .should_browser_enter_foreground = false,
                         .can_show_while_browser_is_backgrounded = false},
                        {.should_serve_at_regular_intervals = false,
                         .opted_in = false,
                         .should_browser_enter_foreground = false,
                         .can_show_while_browser_is_backgrounded = true},
                        {.should_serve_at_regular_intervals = false,
                         .opted_in = false,
                         .should_browser_enter_foreground = true,
                         .can_show_while_browser_is_backgrounded = false},
                        {.should_serve_at_regular_intervals = false,
                         .opted_in = false,
                         .should_browser_enter_foreground = true,
                         .can_show_while_browser_is_backgrounded = true},
                        {.should_serve_at_regular_intervals = false,
                         .opted_in = true,
                         .should_browser_enter_foreground = false,
                         .can_show_while_browser_is_backgrounded = false},
                        {.should_serve_at_regular_intervals = true,
                         .opted_in = true,
                         .should_browser_enter_foreground = false,
                         .can_show_while_browser_is_backgrounded = true},
                        {.should_serve_at_regular_intervals = true,
                         .opted_in = true,
                         .should_browser_enter_foreground = true,
                         .can_show_while_browser_is_backgrounded = false},
                        {.should_serve_at_regular_intervals = true,
                         .opted_in = true,
                         .should_browser_enter_foreground = true,
                         .can_show_while_browser_is_backgrounded = true}};

}  // namespace

class BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUpMocks() override {
    test::SetProfileBooleanPrefValue(prefs::kOptedInToNotificationAds,
                                     GetParam().opted_in);

    test::MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
        ads_client_mock_, GetParam().can_show_while_browser_is_backgrounded);

    test::SetMaximumNotificationAdsPerHour(1);
  }
};

TEST_P(BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest,
       NotificationAdHandler) {
  // Arrange
  if (GetParam().should_browser_enter_foreground) {
    NotifyBrowserDidEnterForeground();
  }

  // Act & Assert
  EXPECT_EQ(GetParam().should_serve_at_regular_intervals,
            ShouldServeAtRegularIntervals());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  const std::string should_serve_at_regular_intervals =
      test_param.param.should_serve_at_regular_intervals
          ? "ShouldServeAtRegularIntervals"
          : "ShouldNotServeAtRegularIntervals";

  const std::string opted_in = test_param.param.opted_in
                                   ? "OptedInToNotificationAds"
                                   : "NotOptedInToNotificationAds";

  const std::string should_browser_enter_foreground =
      test_param.param.should_browser_enter_foreground
          ? "BrowserIsInForeground"
          : "BrowserIsInBackground";

  const std::string can_show_while_browser_is_backgrounded =
      test_param.param.can_show_while_browser_is_backgrounded
          ? "CanShowWhileBrowserIsBackgrounded"
          : "CannotShowWhileBrowserIsBackgrounded";

  return base::ReplaceStringPlaceholders(
      "$1If$2And$3And$4",
      {should_serve_at_regular_intervals, opted_in,
       should_browser_enter_foreground, can_show_while_browser_is_backgrounded},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveAdsNotificationAdHandlerUtilShouldServeAtRegularIntervalsTest,
    ::testing::ValuesIn(kTests),
    TestParamToString);

}  // namespace brave_ads
