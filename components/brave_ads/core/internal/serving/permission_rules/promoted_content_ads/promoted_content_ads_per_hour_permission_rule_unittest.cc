/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/promoted_content_ads/promoted_content_ads_per_hour_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPromotedContentAdsPerHourPermissionRuleTest
    : public test::TestBase {};

TEST_F(BraveAdsPromotedContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfThereAreNoAdEvents) {
  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerHourPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerHour.Get() - 1);

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerHourPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerHourPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(HasPromotedContentAdsPerHourPermission());
}

TEST_F(BraveAdsPromotedContentAdsPerHourPermissionRuleTest,
       ShouldNotAllowIfExceedsCapWithin1Hour) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);
  test::RecordAdEvents(ad, mojom::ConfirmationType::kServedImpression,
                       /*count=*/kMaximumPromotedContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasPromotedContentAdsPerHourPermission());
}

}  // namespace brave_ads
