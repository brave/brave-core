/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/inline_content_ads/inline_content_ads_per_day_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdsPerDayPermissionRuleTest : public UnitTestBase {
 protected:
  const InlineContentAdsPerDayPermissionRule permission_rule_;
};

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get());

  // Arrange
  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsInlineContentAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  test::RecordAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed,
                       /*count=*/kMaximumInlineContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
