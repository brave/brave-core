/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsAdEventBuilderTest, BuildAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/Now());

  // Assert
  EXPECT_THAT(ad_event,
              ::testing::FieldsAre(
                  AdType::kNotificationAd, ConfirmationType::kViewedImpression,
                  kPlacementId, kCreativeInstanceId, kCreativeSetId,
                  kCampaignId, kAdvertiserId, kSegment, /*created_at*/ Now()));
}

TEST_F(BraveAdsAdEventBuilderTest, RebuildAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());

  // Act
  const AdEventInfo rebuilt_ad_event =
      RebuildAdEvent(ad_event, ConfirmationType::kConversion,
                     /*created_at=*/DistantFuture());

  // Assert
  EXPECT_THAT(rebuilt_ad_event,
              ::testing::FieldsAre(AdType::kNotificationAd,
                                   ConfirmationType::kConversion, kPlacementId,
                                   kCreativeInstanceId, kCreativeSetId,
                                   kCampaignId, kAdvertiserId, kSegment,
                                   /*created_at*/ DistantFuture()));
}

}  // namespace brave_ads
