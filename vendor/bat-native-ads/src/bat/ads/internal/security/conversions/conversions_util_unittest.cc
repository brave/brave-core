/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/conversions/conversions_util.h"

#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace security {

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

TEST(BatAdsSecurityConversionsUtilsTest, DoNotSealEnvelopeWithShortMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kShortMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope_optional);
}

TEST(BatAdsSecurityConversionsUtilsTest, DoNotSealEnvelopeWithLongMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kLongMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope_optional);
}

TEST(BatAdsSecurityConversionsUtilsTest, DoNotSealEnvelopeWithInvalidMessage) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kInvalidMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope_optional);
}

TEST(BatAdsSecurityConversionsUtilsTest,
     DoNotSealEnvelopeWithInvalidPublicKey) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kValidMessage;
  verifiable_conversion.public_key = kInvalidAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          SealEnvelope(verifiable_conversion);

  // Assert
  EXPECT_FALSE(verifiable_conversion_envelope_optional);
}

TEST(BatAdsSecurityConversionsUtilsTest, SealEnvelope) {
  // Arrange
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = kValidMessage;
  verifiable_conversion.public_key = kAdvertiserPublicKey;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo>&
      verifiable_conversion_envelope_optional =
          SealEnvelope(verifiable_conversion);
  ASSERT_TRUE(verifiable_conversion_envelope_optional);
  const VerifiableConversionEnvelopeInfo verifiable_conversion_envelope =
      verifiable_conversion_envelope_optional.value();

  const absl::optional<std::string>& message_optional =
      OpenEnvelope(verifiable_conversion_envelope, kAdvertiserSecretKey);
  ASSERT_TRUE(message_optional);
  const std::string message = message_optional.value();

  // Assert
  const std::string expected_message = verifiable_conversion.id;

  EXPECT_EQ(expected_message, message);
}

}  // namespace security
}  // namespace ads
