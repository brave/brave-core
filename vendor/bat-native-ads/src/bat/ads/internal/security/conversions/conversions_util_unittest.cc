/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/conversions/conversions_util.h"

#include <cstdint>
#include <vector>

#include "bat/ads/internal/base64_util.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/security/crypto_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tweetnacl.h"  // NOLINT

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace security {

namespace {

const size_t kCryptoBoxZeroBytes = crypto_box_BOXZEROBYTES;

absl::optional<std::string> EnvelopeOpen(
    const VerifiableConversionEnvelopeInfo envelope,
    const std::string& advertiser_secret_key_base64) {
  if (!envelope.IsValid()) {
    return absl::nullopt;
  }

  std::vector<uint8_t> advertiser_secret_key =
      Base64ToBytes(advertiser_secret_key_base64);
  std::vector<uint8_t> nonce = Base64ToBytes(envelope.nonce);
  std::vector<uint8_t> ciphertext = Base64ToBytes(envelope.ciphertext);
  std::vector<uint8_t> ephemeral_public_key =
      Base64ToBytes(envelope.ephemeral_public_key);

  // API requires 16 leading zero-padding bytes
  ciphertext.insert(ciphertext.begin(), kCryptoBoxZeroBytes, 0);

  std::vector<uint8_t> plaintext =
      Decrypt(ciphertext, nonce, ephemeral_public_key, advertiser_secret_key);
  std::string message = (const char*)&plaintext.front();

  return message;
}

}  // namespace

TEST(BatAdsSecurityConversionsUtilsTest, EnvelopeSealShortMessage) {
  // Arrange
  const std::string advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  const std::string advertiser_secret_key =
      "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";
  const std::string message = "";

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = message;
  verifiable_conversion.public_key = advertiser_public_key;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo> envelope =
      EnvelopeSeal(verifiable_conversion);

  // Assert
  EXPECT_EQ(absl::nullopt, envelope);
}

TEST(BatAdsSecurityConversionsUtilsTest, EnvelopeSealLongMessage) {
  // Arrange
  const std::string advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  const std::string advertiser_secret_key =
      "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";
  const std::string message = "thismessageistoolongthismessageistoolong";

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = message;
  verifiable_conversion.public_key = advertiser_public_key;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo> envelope =
      EnvelopeSeal(verifiable_conversion);

  // Assert
  EXPECT_EQ(absl::nullopt, envelope);
}

TEST(BatAdsSecurityConversionsUtilsTest, EnvelopeSealInvalidMessage) {
  // Arrange
  const std::string advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  const std::string advertiser_secret_key =
      "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";
  const std::string invalid_message = "smart brown foxes 16";

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = invalid_message;
  verifiable_conversion.public_key = advertiser_public_key;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo> envelope =
      EnvelopeSeal(verifiable_conversion);

  // Assert
  EXPECT_EQ(absl::nullopt, envelope);
}

TEST(BatAdsSecurityConversionsUtilsTest, EnvelopeSealWithInvalidPublicKey) {
  // Arrange
  const std::string valid_message = "smartbrownfoxes42";
  const std::string advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYK/INVALID";
  const std::string advertiser_secret_key =
      "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = valid_message;
  verifiable_conversion.public_key = advertiser_public_key;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo> envelope =
      EnvelopeSeal(verifiable_conversion);

  // Assert
  EXPECT_EQ(absl::nullopt, envelope);
}

TEST(BatAdsSecurityConversionsUtilsTest, EnvelopeSeal) {
  // Arrange
  const std::string message = "smartbrownfoxes-42";
  const std::string advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  const std::string advertiser_secret_key =
      "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = message;
  verifiable_conversion.public_key = advertiser_public_key;

  // Act
  const absl::optional<VerifiableConversionEnvelopeInfo> envelope =
      EnvelopeSeal(verifiable_conversion);

  ASSERT_NE(absl::nullopt, envelope);

  const absl::optional<std::string> result =
      EnvelopeOpen(envelope.value(), advertiser_secret_key);

  // Assert
  EXPECT_EQ(message, result.value());
}

}  // namespace security
}  // namespace ads
