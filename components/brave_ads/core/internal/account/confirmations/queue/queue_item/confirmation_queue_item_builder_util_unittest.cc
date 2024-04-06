/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemBuilderUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessClickedConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kClicked));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessDismissedConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kDismissed));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessViewedImpressionConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kViewedImpression));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessServedImpressionConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kServedImpression));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessLandedConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kLanded));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessMarkAdAsInappropriateConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(),
            ProcessConfirmationAt(ConfirmationType::kMarkAdAsInappropriate));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessSavedAdConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kSavedAd));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessLikedAdConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kLikedAd));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessDislikedAdConfirmationAt) {
  // Act & Assert
  EXPECT_EQ(Now(), ProcessConfirmationAt(ConfirmationType::kDislikedAd));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       ProcessConversionConfirmationAt) {
  // Arrange
  const ScopedRandTimeDeltaSetterForTesting scoped_rand_time_delta(
      base::Seconds(21));

  // Act & Assert
  EXPECT_EQ(Now() + base::Seconds(21),
            ProcessConfirmationAt(ConfirmationType::kConversion));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest,
       RetryProcessingConfirmationAfter) {
  // Arrange
  const ScopedRandTimeDeltaSetterForTesting scoped_rand_time_delta(
      base::Seconds(7));

  // Act & Assert
  EXPECT_EQ(base::Seconds(7), RetryProcessingConfirmationAfter());
}

}  // namespace brave_ads
