/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_util.h"

#include <string>

#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_util_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionsUtilTest, DoNotSealEnvelopeWithShortMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id =
      std::string(kMinVerifiableConversionMessageLength - 1, '-');
  verifiable_conversion.public_key = kConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_FALSE(SealEnvelope(verifiable_conversion));
}

TEST(BraveAdsConversionsUtilTest, DoNotSealEnvelopeWithLongMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id =
      std::string(kMaxVerifiableConversionMessageLength + 1, '-');
  verifiable_conversion.public_key = kConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_FALSE(SealEnvelope(verifiable_conversion));
}

TEST(BraveAdsConversionsUtilTest, DoNotSealEnvelopeWithInvalidMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kInvalidConversionId;
  verifiable_conversion.public_key = kConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_FALSE(SealEnvelope(verifiable_conversion));
}

TEST(BraveAdsConversionsUtilTest, DoNotSealEnvelopeWithInvalidPublicKey) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kConversionId;
  verifiable_conversion.public_key = kInvalidConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_FALSE(SealEnvelope(verifiable_conversion));
}

TEST(BraveAdsConversionsUtilTest, SealEnvelope) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kConversionId;
  verifiable_conversion.public_key = kConversionAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);
  ASSERT_TRUE(verifiable_conversion_envelope);

  // Assert
  EXPECT_EQ(verifiable_conversion.id,
            OpenEnvelope(*verifiable_conversion_envelope,
                         kConversionAdvertiserSecretKey));
}

}  // namespace brave_ads
