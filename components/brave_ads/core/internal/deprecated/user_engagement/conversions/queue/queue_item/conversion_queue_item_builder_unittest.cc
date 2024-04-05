/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/conversion/conversion_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueItemBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConversionQueueItemBuilderTest, BuildConversionQueueItem) {
  // Arrange
  const ConversionInfo conversion = test::BuildConversion(
      AdType::kSearchResultAd, ConfirmationType::kViewedImpression,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  ConversionQueueItemInfo expected_conversion_queue_item;
  expected_conversion_queue_item.conversion = conversion;
  expected_conversion_queue_item.process_at = Now();
  expected_conversion_queue_item.was_processed = false;
  EXPECT_EQ(expected_conversion_queue_item,
            BuildConversionQueueItem(conversion, /*process_at=*/Now()));
}

TEST_F(BraveAdsConversionQueueItemBuilderTest,
       BuildVerifiableConversionQueueItem) {
  // Arrange
  const ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);

  // Act & Assert
  ConversionQueueItemInfo expected_conversion_queue_item;
  expected_conversion_queue_item.conversion = conversion;
  expected_conversion_queue_item.process_at = Now();
  expected_conversion_queue_item.was_processed = false;
  EXPECT_EQ(expected_conversion_queue_item,
            BuildConversionQueueItem(conversion, /*process_at=*/Now()));
}

}  // namespace brave_ads
