/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest
    : public test::TestBase {};

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasNotificationAdMinimumWaitTimePermission());
}

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::SetMaximumNotificationAdsPerHour(5);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kServedImpression);

  AdvanceClockBy(base::Minutes(12));

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdMinimumWaitTimePermission());
}

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       ShouldNotAllowIfExceedsCap) {
  // Arrange
  test::SetMaximumNotificationAdsPerHour(5);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kServedImpression);

  AdvanceClockBy(base::Minutes(12) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasNotificationAdMinimumWaitTimePermission());
}

}  // namespace brave_ads
