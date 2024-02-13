/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsPerDayPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNotificationAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNotificationAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPerDayPermission());
}

TEST_F(BraveAdsNotificationAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNotificationAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasNotificationAdsPerDayPermission());
}

}  // namespace brave_ads
