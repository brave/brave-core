/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsPerDayPermissionRuleTest : public test::TestBase {
};

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNotificationAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNotificationAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/false);
  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumNotificationAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasNotificationAdsPerDayPermission());
}

}  // namespace brave_ads
