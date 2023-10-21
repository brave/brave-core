/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsPerHourPermissionRuleTest : public UnitTestBase {
 protected:
  const NotificationAdsPerHourPermissionRule permission_rule_;
};

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
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
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
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
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const int ads_per_hour = kDefaultNotificationAdsPerHour.Get();

  test::SetMaximumNotificationAdsPerHour(ads_per_hour);

  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/ads_per_hour - 1);

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
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
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
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
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
