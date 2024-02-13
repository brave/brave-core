/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdsPerDayPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerDayPermission());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerDayPermission());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get());

  // Arrange
  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerDayPermission());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasInlineContentAdsPerDayPermission());
}

}  // namespace brave_ads
