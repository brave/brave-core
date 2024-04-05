/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/conversion/conversion_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_util_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueItemDelayTest : public UnitTestBase {};

TEST_F(BraveAdsConversionQueueItemDelayTest,
       CalculateDelayBeforeProcessingConversionQueueItem) {
  // Arrange
  const ConversionInfo conversion = test::BuildConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_use_random_uuids=*/false);
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
  const ConversionInfo conversion = test::BuildConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion, /*process_at=*/DistantPast());

  // Act & Assert
  EXPECT_EQ(
      kMinimumDelayBeforeProcessingConversionQueueItem,
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item));
}

TEST_F(BraveAdsConversionQueueItemDelayTest,
       CalculateMinimumDelayBeforeProcessingConversionQueueItem) {
  // Arrange
  const ConversionInfo conversion = test::BuildConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion,
                               /*process_at=*/Now() + base::Milliseconds(1));

  // Act & Assert
  EXPECT_EQ(
      kMinimumDelayBeforeProcessingConversionQueueItem,
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item));
}

}  // namespace brave_ads
