/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/common/crypto/key_pair_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tweetnacl.h"  // NOLINT

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::crypto {

namespace {

constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
constexpr char kPublicKeyBase64[] =
    "5LmgyD6OG0qcVeRgTzk3IWbzSWjemE4KpjTRtRW4eRk=";
constexpr char kSecretKeyBase64[] =
    R"(oyd1rHNB5xHU6TzPSO/MUUfUJNHiol1ExFHMMKV/7dvkuaDIPo4bSpxV5GBPOTchZvNJaN6YTgqmNNG1Fbh5GQ==)";

}  // namespace

TEST(BraveAdsCryptoUtilTest, Sha256) {
  // Act
  const std::vector<uint8_t> sha256 = Sha256(kMessage);

  // Assert
  EXPECT_EQ("16j7swfXgJRpypq8sAguT41WUeRtPNt2LQLQvzfJ5ZI=",
            base::Base64Encode(sha256));
}

TEST(BraveAdsCryptoUtilTest, Sha256WithEmptyString) {
  // Act
  const std::vector<uint8_t> sha256 = Sha256(/*value=*/"");

  // Assert
  EXPECT_EQ("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=",
            base::Base64Encode(sha256));
}

TEST(BraveAdsCryptoUtilTest, GenerateSignKeyPairFromSeed) {
  // Arrange
  const std::optional<std::vector<uint8_t>> seed =
      base::Base64Decode("x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=");
  ASSERT_TRUE(seed);

  // Act
  const std::optional<KeyPairInfo> key_pair =
      GenerateSignKeyPairFromSeed(*seed);
  ASSERT_TRUE(key_pair);
  ASSERT_THAT(key_pair->public_key,
              ::testing::SizeIs(crypto_sign_ed25519_PUBLICKEYBYTES));
  ASSERT_THAT(key_pair->secret_key,
              ::testing::SizeIs(crypto_sign_ed25519_SECRETKEYBYTES));

  // Assert
  EXPECT_TRUE(key_pair->IsValid());
}

TEST(BraveAdsCryptoUtilTest, GenerateBoxKeyPair) {
  // Act
  const KeyPairInfo key_pair = GenerateBoxKeyPair();
  ASSERT_THAT(key_pair.public_key,
              ::testing::SizeIs(crypto_box_PUBLICKEYBYTES));
  ASSERT_THAT(key_pair.secret_key,
              ::testing::SizeIs(crypto_box_SECRETKEYBYTES));

  // Assert
  EXPECT_TRUE(key_pair.IsValid());
}

TEST(BraveAdsCryptoUtilTest, GenerateRandomNonce) {
  // Act & Assert
  EXPECT_THAT(GenerateRandomNonce(), ::testing::SizeIs(crypto_box_NONCEBYTES));
}

TEST(BraveAdsCryptoUtilTest, Sign) {
  // Act
  const std::optional<std::string> signature = Sign(kMessage, kSecretKeyBase64);

  // Assert
  EXPECT_TRUE(signature);
}

TEST(BraveAdsCryptoUtilTest, Verify) {
  // Act
  const std::optional<std::string> signature = Sign(kMessage, kSecretKeyBase64);
  ASSERT_TRUE(signature);

  // Assert
  EXPECT_TRUE(Verify(kMessage, kPublicKeyBase64, *signature));
}

TEST(BraveAdsCryptoUtilTest, EncryptAndDecrypt) {
  // Arrange
  const KeyPairInfo key_pair = GenerateBoxKeyPair();
  const KeyPairInfo ephemeral_key_pair = GenerateBoxKeyPair();
  const std::vector<uint8_t> nonce = GenerateRandomNonce();
  const std::vector<uint8_t> plaintext(
      kMessage, UNSAFE_TODO(kMessage + std::size(kMessage)));

  // Act
  const std::vector<uint8_t> ciphertext = Encrypt(
      plaintext, nonce, key_pair.public_key, ephemeral_key_pair.secret_key);

  // Assert
  EXPECT_EQ(plaintext, Decrypt(ciphertext, nonce, ephemeral_key_pair.public_key,
                               key_pair.secret_key));
}

}  // namespace brave_ads::crypto
