/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_util.h"

#include <string>

#include "bat/ads/internal/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::security {

namespace {

constexpr char kAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
constexpr char kInvalidAdvertiserPublicKey[] = "invalid";
constexpr char kAdvertiserSecretKey[] =
    "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

constexpr char kShortMessage[] = "";
constexpr char kLongMessage[] = "thismessageistoolongthismessageistoolong";
constexpr char kValidMessage[] = "smartbrownfoxes42";
constexpr char kInvalidMessage[] = "smart brown foxes 16";

}  // namespace

TEST(BatAdsConversionsUtilTest, DoNotSealEnvelopeWithShortMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kShortMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BatAdsConversionsUtilTest, DoNotSealEnvelopeWithLongMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kLongMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BatAdsConversionsUtilTest, DoNotSealEnvelopeWithInvalidMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kInvalidMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BatAdsConversionsUtilTest, DoNotSealEnvelopeWithInvalidPublicKey) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kValidMessage;
  verifiable_conversion.public_key = kInvalidAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope);
}

TEST(BatAdsConversionsUtilTest, SealEnvelope) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kValidMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>
      verifiable_conversion_envelope = SealEnvelope(verifiable_conversion);
  ASSERT_TRUE(verifiable_conversion_envelope);

  const absl::optional<std::string> message =
      OpenEnvelope(*verifiable_conversion_envelope, kAdvertiserSecretKey);
  ASSERT_TRUE(message);

  // Assert
  EXPECT_EQ(verifiable_conversion.id, *message);
}

}  // namespace ads::security
