/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util.h"

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     SealVerifiableConversionEnvelope) {
  // Arrange
  const VerifiableConversionInfo verifiable_conversion{
      kVerifiableConversionId, kVerifiableConversionAdvertiserPublicKey};

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope =
          SealVerifiableConversionEnvelope(verifiable_conversion);
  ASSERT_TRUE(verifiable_conversion_envelope);

  // Assert
  EXPECT_EQ(verifiable_conversion.id,
            test::OpenVerifiableConversionEnvelope(
                *verifiable_conversion_envelope,
                kVerifiableConversionAdvertiserSecretKey));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithShortMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id =
      std::string(kMinVerifiableConversionEnvelopeMessageLength - 1, '-');
  verifiable_conversion.advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(verifiable_conversion));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithLongMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id =
      std::string(kMaxVerifiableConversionEnvelopeMessageLength + 1, '-');
  verifiable_conversion.advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(verifiable_conversion));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithInvalidMessage) {
  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(
      VerifiableConversionInfo{kInvalidVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey}));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithInvalidPublicKey) {
  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(VerifiableConversionInfo{
      kVerifiableConversionId,
      kInvalidVerifiableConversionAdvertiserPublicKey}));
}

}  // namespace brave_ads
