/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/units/ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionBuilderTest, BuildConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  // Act & Assert
  ConversionInfo expected_conversion;
  expected_conversion.ad_type = AdType::kNotificationAd;
  expected_conversion.creative_instance_id = kCreativeInstanceId;
  expected_conversion.creative_set_id = kCreativeSetId;
  expected_conversion.campaign_id = kCampaignId;
  expected_conversion.advertiser_id = kAdvertiserId;
  expected_conversion.segment = kSegment;
  expected_conversion.action_type = ConversionActionType::kViewThrough;
  EXPECT_EQ(expected_conversion,
            BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                         /*created_at=*/Now()),
                            /*verifiable_conversion=*/absl::nullopt));
}

TEST(BraveAdsConversionBuilderTest, BuildVerifiableConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  // Act & Assert
  ConversionInfo expected_conversion;
  expected_conversion.ad_type = AdType::kNotificationAd;
  expected_conversion.creative_instance_id = kCreativeInstanceId;
  expected_conversion.creative_set_id = kCreativeSetId;
  expected_conversion.campaign_id = kCampaignId;
  expected_conversion.advertiser_id = kAdvertiserId;
  expected_conversion.segment = kSegment;
  expected_conversion.action_type = ConversionActionType::kViewThrough;
  expected_conversion.verifiable = VerifiableConversionInfo{
      kVerifiableConversionId, kVerifiableConversionAdvertiserPublicKey};
  EXPECT_EQ(expected_conversion,
            BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                         /*created_at=*/Now()),
                            VerifiableConversionInfo{
                                kVerifiableConversionId,
                                kVerifiableConversionAdvertiserPublicKey}));
}

}  // namespace brave_ads
