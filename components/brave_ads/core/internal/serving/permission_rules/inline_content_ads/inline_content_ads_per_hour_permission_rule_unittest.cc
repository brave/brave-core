/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/inline_content_ads/inline_content_ads_per_hour_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdsPerHourPermissionRuleTest
    : public test::TestBase {};

TEST_F(BraveAdsInlineContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerHourPermission());
}

TEST_F(BraveAdsInlineContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const InlineContentAdInfo ad =
      test::BuildInlineContentAd(/*should_generate_random_uuids=*/false);

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumInlineContentAdsPerHour.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerHourPermission());
}

TEST_F(BraveAdsInlineContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const InlineContentAdInfo ad =
      test::BuildInlineContentAd(/*should_generate_random_uuids=*/false);

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumInlineContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(HasInlineContentAdsPerHourPermission());
}

TEST_F(BraveAdsInlineContentAdsPerHourPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Hour) {
  // Arrange
  const InlineContentAdInfo ad =
      test::BuildInlineContentAd(/*should_generate_random_uuids=*/false);

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       /*count=*/kMaximumInlineContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasInlineContentAdsPerHourPermission());
}

}  // namespace brave_ads
