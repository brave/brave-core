/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsAdEventBuilderTest, BuildAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  // Act & Assert
  AdEventInfo expected_ad_event;
  expected_ad_event.type = AdType::kNotificationAd;
  expected_ad_event.confirmation_type = ConfirmationType::kViewed;
  expected_ad_event.placement_id = kPlacementId;
  expected_ad_event.creative_instance_id = kCreativeInstanceId;
  expected_ad_event.creative_set_id = kCreativeSetId;
  expected_ad_event.campaign_id = kCampaignId;
  expected_ad_event.advertiser_id = kAdvertiserId;
  expected_ad_event.segment = kSegment;
  expected_ad_event.created_at = Now();
  EXPECT_EQ(expected_ad_event,
            BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()));
}

TEST_F(BraveAdsAdEventBuilderTest, RebuildAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now());

  // Act & Assert
  AdEventInfo expected_rebuilt_ad_event;
  expected_rebuilt_ad_event.type = AdType::kNotificationAd;
  expected_rebuilt_ad_event.confirmation_type = ConfirmationType::kConversion;
  expected_rebuilt_ad_event.placement_id = kPlacementId;
  expected_rebuilt_ad_event.creative_instance_id = kCreativeInstanceId;
  expected_rebuilt_ad_event.creative_set_id = kCreativeSetId;
  expected_rebuilt_ad_event.campaign_id = kCampaignId;
  expected_rebuilt_ad_event.advertiser_id = kAdvertiserId;
  expected_rebuilt_ad_event.segment = kSegment;
  expected_rebuilt_ad_event.created_at = DistantFuture();
  EXPECT_EQ(expected_rebuilt_ad_event,
            RebuildAdEvent(ad_event, ConfirmationType::kConversion,
                           /*created_at=*/DistantFuture()));
}

}  // namespace brave_ads
