/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPromotedContentAdsPerDayPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasPromotedContentAdsPerDayPermission());
}

}  // namespace brave_ads
