/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util.h"

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_util_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     SealVerifiableConversionEnvelope) {
  // Arrange
  const VerifiableConversionInfo verifiable_conversion{
      test::kVerifiableConversionId,
      test::kVerifiableConversionAdvertiserPublicKeyBase64};

  // Act
  const std::optional<VerifiableConversionEnvelopeInfo>
      sealed_verifiable_conversion_envelope =
          SealVerifiableConversionEnvelope(verifiable_conversion);
  ASSERT_TRUE(sealed_verifiable_conversion_envelope);

  // Assert
  EXPECT_EQ(verifiable_conversion.id,
            test::OpenVerifiableConversionEnvelope(
                *sealed_verifiable_conversion_envelope,
                test::kVerifiableConversionAdvertiserSecretKeyBase64));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithShortMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id =
      std::string(kMinVerifiableConversionEnvelopeMessageLength - 1, '-');
  verifiable_conversion.advertiser_public_key_base64 =
      test::kVerifiableConversionAdvertiserPublicKeyBase64;

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
      test::kVerifiableConversionAdvertiserPublicKeyBase64;

  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(verifiable_conversion));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithInvalidMessage) {
  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(VerifiableConversionInfo{
      test::kInvalidVerifiableConversionId,
      test::kVerifiableConversionAdvertiserPublicKeyBase64}));
}

TEST(BraveAdsVerifiableConversionEnvelopeUtilTest,
     DoNotSealEnvelopeWithInvalidPublicKey) {
  // Act & Assert
  EXPECT_FALSE(SealVerifiableConversionEnvelope(VerifiableConversionInfo{
      test::kVerifiableConversionId,
      test::kInvalidVerifiableConversionAdvertiserPublicKeyBase64}));
}

}  // namespace brave_ads
