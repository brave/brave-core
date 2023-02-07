/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/crypto/crypto_util.h"

#include "base/base64.h"
#include "bat/ads/internal/common/crypto/key_pair_info.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "tweetnacl.h"                          // NOLINT

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::crypto {

namespace {

constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
constexpr char kPublicKey[] = "5LmgyD6OG0qcVeRgTzk3IWbzSWjemE4KpjTRtRW4eRk=";
constexpr char kSecretKey[] =
    R"(oyd1rHNB5xHU6TzPSO/MUUfUJNHiol1ExFHMMKV/7dvkuaDIPo4bSpxV5GBPOTchZvNJaN6YTgqmNNG1Fbh5GQ==)";

}  // namespace

TEST(BatAdsCryptoUtilTest, Sha256) {
  // Arrange

  // Act
  const std::vector<uint8_t> sha256 = Sha256(kMessage);

  // Assert
  const std::string expected_sha256_base64 =
      "16j7swfXgJRpypq8sAguT41WUeRtPNt2LQLQvzfJ5ZI=";

  EXPECT_EQ(expected_sha256_base64, base::Base64Encode(sha256));
}

TEST(BatAdsCryptoUtilTest, Sha256WithEmptyString) {
  // Arrange
  const std::string value;

  // Act
  const std::vector<uint8_t> sha256 = Sha256(value);

  // Assert
  const std::string expected_sha256_base64 =
      "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";
  EXPECT_EQ(expected_sha256_base64, base::Base64Encode(sha256));
}

TEST(BatAdsCryptoUtilTest, GenerateSignKeyPairFromSeed) {
  // Arrange
  const absl::optional<std::vector<uint8_t>> seed =
      base::Base64Decode("x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=");
  ASSERT_TRUE(seed);

  // Act
  const absl::optional<KeyPairInfo> key_pair =
      GenerateSignKeyPairFromSeed(*seed);
  ASSERT_TRUE(key_pair);

  // Assert
  ASSERT_EQ(crypto_sign_ed25519_PUBLICKEYBYTES,
            static_cast<int>(key_pair->public_key.size()));
  ASSERT_EQ(crypto_sign_ed25519_SECRETKEYBYTES,
            static_cast<int>(key_pair->secret_key.size()));
  EXPECT_TRUE(key_pair->IsValid());
}

TEST(BatAdsCryptoUtilTest, GenerateBoxKeyPair) {
  // Arrange

  // Act
  const KeyPairInfo key_pair = GenerateBoxKeyPair();

  // Assert
  ASSERT_EQ(crypto_box_PUBLICKEYBYTES,
            static_cast<int>(key_pair.public_key.size()));
  ASSERT_EQ(crypto_box_SECRETKEYBYTES,
            static_cast<int>(key_pair.secret_key.size()));
  EXPECT_TRUE(key_pair.IsValid());
}

TEST(BatAdsCryptoUtilTest, GenerateRandomNonce) {
  // Arrange

  // Act
  const std::vector<uint8_t> nonce = GenerateRandomNonce();

  // Assert
  EXPECT_EQ(crypto_box_NONCEBYTES, static_cast<int>(nonce.size()));
}

TEST(BatAdsCryptoUtilTest, Sign) {
  // Arrange

  // Act
  const absl::optional<std::string> signature = Sign(kMessage, kSecretKey);
  ASSERT_TRUE(signature);

  // Assert
  const std::string expected_signature =
      "t4VwMNwX7hsAHQVXNGl3nGWj6LtCYSacEN/J0xKtXK6sQ5uBRB3m9kE6mVPHj6/"
      "cv90OIdvrVcrl+eZm60FbAQ==";
  EXPECT_EQ(expected_signature, *signature);
  EXPECT_TRUE(Verify(kMessage, kPublicKey, *signature));
}

TEST(BatAdsCryptoUtilTest, Encrypt) {
  // Arrange
  const KeyPairInfo key_pair = GenerateBoxKeyPair();
  const KeyPairInfo ephemeral_key_pair = GenerateBoxKeyPair();
  const std::vector<uint8_t> nonce = GenerateRandomNonce();
  const std::string message = kMessage;
  const std::vector<uint8_t> plaintext(message.cbegin(), message.cend());

  // Act
  const std::vector<uint8_t> ciphertext = Encrypt(
      plaintext, nonce, key_pair.public_key, ephemeral_key_pair.secret_key);

  const std::vector<uint8_t> decrypted_plaintext = Decrypt(
      ciphertext, nonce, ephemeral_key_pair.public_key, key_pair.secret_key);

  // Assert
  EXPECT_EQ(plaintext, decrypted_plaintext);
}

}  // namespace ads::crypto
