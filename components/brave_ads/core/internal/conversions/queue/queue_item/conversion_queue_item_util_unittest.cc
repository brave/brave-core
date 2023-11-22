/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueItemDelayTest : public UnitTestBase {};

TEST_F(BraveAdsConversionQueueItemDelayTest,
       CalculateDelayBeforeProcessingConversionQueueItem) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/absl::nullopt);
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion,
                               /*process_at=*/Now() + base::Hours(1));

  // Act & Assert
  EXPECT_EQ(base::Hours(1), CalculateDelayBeforeProcessingConversionQueueItem(
                                conversion_queue_item));
}

TEST_F(BraveAdsConversionQueueItemDelayTest,
       CalculateDelayBeforeProcessingPastDueConversionQueueItem) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at=*/DistantPast()),
                      /*verifiable_conversion=*/absl::nullopt);
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion, /*process_at=*/DistantPast());

  // Act & Assert
  EXPECT_EQ(
      kMinimumDelayBeforeProcessingQueueItem,
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item));
}

TEST_F(BraveAdsConversionQueueItemDelayTest,
       CalculateMinimumDelayBeforeProcessingConversionQueueItem) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/absl::nullopt);
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion,
                               /*process_at=*/Now() + base::Milliseconds(1));

  // Act & Assert
  EXPECT_EQ(
      kMinimumDelayBeforeProcessingQueueItem,
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item));
}

}  // namespace brave_ads
