/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdsPerDayPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(ShouldAllowNewTabPageAdsPerDay());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(ShouldAllowNewTabPageAdsPerDay());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(ShouldAllowNewTabPageAdsPerDay());
}

TEST_F(BraveAdsNewTabPageAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed,
                       /*count=*/kMaximumNewTabPageAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(ShouldAllowNewTabPageAdsPerDay());
}

}  // namespace brave_ads
