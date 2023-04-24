/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data_util.h"

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::user_data {

TEST(BraveAdsConversionUserDataUtilTest, GetEnvelope) {
  // Arrange
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_guids*/ false);

  // Act
  const absl::optional<security::VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = GetEnvelope(conversion_queue_item);

  // Assert
  EXPECT_TRUE(verifiable_conversion_envelope);
}

TEST(BraveAdsConversionUserDataUtilTest,
     DoNotGetEnvelopeIfConversionIdIsEmpty) {
  // Arrange
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kEmptyConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_guids*/ false);

  // Act
  const absl::optional<security::VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = GetEnvelope(conversion_queue_item);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BraveAdsConversionUserDataUtilTest,
     DoNotGetEnvelopeIfAdvertiserPublicKeyIsEmpty) {
  // Arrange
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kEmptyConversionAdvertiserPublicKey,
                               /*should_use_random_guids*/ false);

  // Act
  const absl::optional<security::VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = GetEnvelope(conversion_queue_item);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BraveAdsConversionUserDataUtilTest,
     DoNotGetEnvelopeIfConversionIdAndAdvertiserPublicKeyAreEmpty) {
  // Arrange
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kEmptyConversionId,
                               kEmptyConversionAdvertiserPublicKey,
                               /*should_use_random_guids*/ false);

  // Act
  const absl::optional<security::VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = GetEnvelope(conversion_queue_item);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

}  // namespace brave_ads::user_data
