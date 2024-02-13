/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsPerHourPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerHourPermission());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAlwaysAllowOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour);

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerHourPermission());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAlwaysAllowOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour);

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerHourPermission());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour - 1);

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerHourPermission());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerHourPermission());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Hour) {
  // Arrange
  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasNotificationAdsPerHourPermission());
}

}  // namespace brave_ads
