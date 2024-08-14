/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionBuilderTest, BuildConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act
  const ConversionInfo conversion =
      BuildConversion(ad_event, /*verifiable_conversion=*/std::nullopt);

  // Assert
  EXPECT_THAT(conversion,
              ::testing::FieldsAre(
                  AdType::kNotificationAd, test::kCreativeInstanceId,
                  test::kCreativeSetId, test::kCampaignId, test::kAdvertiserId,
                  test::kSegment, ConversionActionType::kViewThrough,
                  /*verifable*/ std::nullopt));
}

TEST(BraveAdsConversionBuilderTest, BuildVerifiableConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act
  const ConversionInfo conversion = BuildConversion(
      ad_event, VerifiableConversionInfo{
                    test::kVerifiableConversionId,
                    test::kVerifiableConversionAdvertiserPublicKeyBase64});

  // Assert
  EXPECT_THAT(conversion,
              ::testing::FieldsAre(
                  AdType::kNotificationAd, test::kCreativeInstanceId,
                  test::kCreativeSetId, test::kCampaignId, test::kAdvertiserId,
                  test::kSegment, ConversionActionType::kViewThrough,
                  VerifiableConversionInfo{
                      test::kVerifiableConversionId,
                      test::kVerifiableConversionAdvertiserPublicKeyBase64}));
}

}  // namespace brave_ads
