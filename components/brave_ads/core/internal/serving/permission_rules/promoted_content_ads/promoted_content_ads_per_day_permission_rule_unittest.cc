/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/promoted_content_ads/promoted_content_ads_per_day_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPromotedContentAdsPerDayPermissionRuleTest
    : public test::TestBase {};

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Day) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerDayPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerDayPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Day) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasPromotedContentAdsPerDayPermission());
}

}  // namespace brave_ads
