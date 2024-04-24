/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_validation_util.h"

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/conversion/conversion_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "testing/gtest/include/gtest/gtest.h"
// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsValidationUtilTest, InvalidConversionQueueItem) {
  // Arrange
  ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);
  conversion.ad_type = AdType::kUndefined;

  ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);
  conversion_queue_items[0].process_at.reset();

  // Act & Assert
  EXPECT_EQ("ad_type,process_at", GetConversionQueueItemInvalidFieldsNames(
                                      conversion_queue_items[0]));
}

TEST(BraveAdsValidationUtilTest, ValidConversionQueueItem) {
  // Arrange
  const ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);

  // Act & Assert
  EXPECT_EQ(
      "", GetConversionQueueItemInvalidFieldsNames(conversion_queue_items[0]));
}

}  // namespace brave_ads
