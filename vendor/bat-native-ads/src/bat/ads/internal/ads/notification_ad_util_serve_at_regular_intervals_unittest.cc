/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/notification_ad_util.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

struct ParamInfo final {
  bool should_serve;
  bool is_browser_active;
  bool can_show_background_notifications;
  int ads_per_hour;
  bool should_serve_at_regular_intervals;
};

const ParamInfo kTests[] = {
    {/* should_serve */ false, /* is_browser_active */ false,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ false,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ false,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ false,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ true,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ true,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ true,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ false, /* is_browser_active */ true,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ false,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ false,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ false,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ false,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ true},
    {/* should_serve */ true, /* is_browser_active */ true,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ true,
     /* can_show_background_notifications */ false, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ true},
    {/* should_serve */ true, /* is_browser_active */ true,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 0,
     /* should_serve_at_regular_intervals */ false},
    {/* should_serve */ true, /* is_browser_active */ true,
     /* can_show_background_notifications */ true, /* ads_per_hour */ 1,
     /* should_serve_at_regular_intervals */ true}};

}  // namespace

class BatAdsNotificationAdUtilServeAtRegularIntervalsTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  BatAdsNotificationAdUtilServeAtRegularIntervalsTest() = default;

  ~BatAdsNotificationAdUtilServeAtRegularIntervalsTest() override = default;

  void SetUpMocks() override {
    const ParamInfo param = GetParam();

    AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled,
                                                   param.should_serve);

    MockIsBrowserActive(ads_client_mock_, param.is_browser_active);

    MockCanShowBackgroundNotifications(ads_client_mock_,
                                       param.can_show_background_notifications);

    AdsClientHelper::GetInstance()->SetInt64Pref(prefs::kAdsPerHour,
                                                 param.ads_per_hour);
  }
};

TEST_P(BatAdsNotificationAdUtilServeAtRegularIntervalsTest, NotificationAd) {
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

  const std::string should_serve =
      test_param.param.should_serve ? "ShouldServe" : "ShouldNotServe";

  const std::string is_browser_active = test_param.param.is_browser_active
                                            ? "BrowserIsActive"
                                            : "BrowserIsInactive";

  const std::string can_show_background_notifications =
      test_param.param.can_show_background_notifications
          ? "BackgroundNotificationsAreEnabled"
          : "BackgroundNotificationsAreDisabled";

  const std::string ads_per_hour =
      base::StringPrintf("%dAdsPerHour", test_param.param.ads_per_hour);

  return base::StringPrintf(
      "%sIf%sAnd%sAnd%sAnd%s", should_serve_at_regular_intervals.c_str(),
      should_serve.c_str(), is_browser_active.c_str(),
      can_show_background_notifications.c_str(), ads_per_hour.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsNotificationAdUtilServeAtRegularIntervalsTest,
                         testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace ads
