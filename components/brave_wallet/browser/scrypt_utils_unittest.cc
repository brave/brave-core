/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/scrypt_utils.h"

#include <array>
#include <vector>

#include "base/containers/span.h"
#include "crypto/kdf.h"
#include "crypto/random.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(ScryptUtilsTest, EncryptDecrypt_BasicRoundtrip) {
  const std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03, 0x04, 0x05};
  const std::string password = "test_password";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,                         // n
      .block_size = 8,                       // r
      .parallelization = 1,                  // p
      .max_memory_bytes = 64 * 1024 * 1024,  // 64 MB
  };

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  crypto::RandBytes(salt);
  crypto::RandBytes(nonce);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto encrypted = ScryptEncrypt(plaintext, *derived_key, nonce);

  auto decrypt_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto decrypted = ScryptDecrypt(*encrypted, nonce, *decrypt_key);
  EXPECT_EQ(*decrypted, plaintext);
}

TEST(ScryptUtilsTest, EncryptDecrypt_EmptyPlaintext) {
  const std::vector<uint8_t> plaintext = {};
  const std::string password = "password";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  crypto::RandBytes(salt);
  crypto::RandBytes(nonce);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto encrypted = ScryptEncrypt(plaintext, *derived_key, nonce);

  auto decrypt_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto decrypted = ScryptDecrypt(*encrypted, nonce, *decrypt_key);
  EXPECT_EQ(*decrypted, plaintext);
}

TEST(ScryptUtilsTest, EncryptDecrypt_WithProvidedSaltAndNonce) {
  const std::vector<uint8_t> plaintext = {0xAA, 0xBB, 0xCC, 0xDD};
  const std::string password = "password123";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  // Provide specific salt and nonce for testing.
  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  salt.fill(0x42);
  nonce.fill(0x99);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto encrypted = ScryptEncrypt(plaintext, *derived_key, nonce);

  auto decrypted = ScryptDecrypt(*encrypted, nonce, *derived_key);
  EXPECT_EQ(*decrypted, plaintext);
}

TEST(ScryptUtilsTest, EncryptDecrypt_WrongPassword) {
  const std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03};
  const std::string password = "correct_password";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  crypto::RandBytes(salt);
  crypto::RandBytes(nonce);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  auto encrypted = ScryptEncrypt(plaintext, *derived_key, nonce);

  const std::string wrong_password = "wrong_password";
  auto wrong_key = ScryptDeriveKey(wrong_password, salt, scrypt_params);
  auto decrypted = ScryptDecrypt(*encrypted, nonce, *wrong_key);
  EXPECT_FALSE(decrypted.has_value());
}

TEST(ScryptUtilsTest, EncryptDecrypt_WrongScryptParams) {
  const std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03};
  const std::string password = "password";

  // Encrypt with one set of params.
  crypto::kdf::ScryptParams scrypt_params1 = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  // Derive encryption key with first set of params
  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  crypto::RandBytes(salt);
  crypto::RandBytes(nonce);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params1);
  auto encrypted = ScryptEncrypt(plaintext, *derived_key, nonce);

  // Try to decrypt with different scrypt params (should fail - different key).
  crypto::kdf::ScryptParams scrypt_params2 = {
      .cost = 32768,  // Different cost
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  auto wrong_key = ScryptDeriveKey(password, salt, scrypt_params2);
  auto decrypted = ScryptDecrypt(*encrypted, nonce, *wrong_key);
  EXPECT_FALSE(decrypted.has_value());
}

TEST(ScryptUtilsTest, EncryptDecrypt_EmptyPassword) {
  const std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03};
  const std::string password = "";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  crypto::RandBytes(salt);
  crypto::RandBytes(nonce);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  EXPECT_FALSE(derived_key.has_value());
}

TEST(ScryptUtilsTest, EncryptDecrypt_DeterministicWithSameSaltAndNonce) {
  const std::vector<uint8_t> plaintext = {0xAA, 0xBB, 0xCC};
  const std::string password = "password";
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  salt.fill(0x11);
  nonce.fill(0x22);

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);

  auto encrypted1 = ScryptEncrypt(plaintext, *derived_key, nonce);
  auto encrypted2 = ScryptEncrypt(plaintext, *derived_key, nonce);

  // Results should be identical (deterministic encryption).
  EXPECT_EQ(encrypted1, encrypted2);

  // Both should decrypt to the same plaintext.
  auto decrypted1 = ScryptDecrypt(*encrypted1, nonce, *derived_key);
  auto decrypted2 = ScryptDecrypt(*encrypted2, nonce, *derived_key);
  EXPECT_EQ(*decrypted1, plaintext);
  EXPECT_EQ(*decrypted2, plaintext);
}

// https://datatracker.ietf.org/doc/html/rfc7914#section-12
TEST(ScryptUtilsTest, DeriveKey_TestVector1) {
  const std::string password = "password";
  const std::string salt_str = "NaCl";

  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 1024,                           // N
      .block_size = 8,                        // r
      .parallelization = 16,                  // p
      .max_memory_bytes = 512 * 1024 * 1024,  // 512 MB
  };

  auto derived_key =
      ScryptDeriveKey(password, base::as_byte_span(salt_str), scrypt_params);
  ASSERT_TRUE(derived_key.has_value());

  // We use the first 32 bytes for comparison (dkLen=64, but we only need 32).
  std::array<uint8_t, kScryptKeyBytes> expected_key = {
      0xfd, 0xba, 0xbe, 0x1c, 0x9d, 0x34, 0x72, 0x00, 0x78, 0x56, 0xe7,
      0x19, 0x0d, 0x01, 0xe9, 0xfe, 0x7c, 0x6a, 0xd7, 0xcb, 0xc8, 0x23,
      0x78, 0x30, 0xe7, 0x73, 0x76, 0x63, 0x4b, 0x37, 0x31, 0x62};

  EXPECT_EQ(*derived_key, expected_key);
}

// https://datatracker.ietf.org/doc/html/rfc7914#section-12
TEST(ScryptUtilsTest, DeriveKey_TestVector2) {
  const std::string password = "pleaseletmein";
  const std::string salt_str = "SodiumChloride";

  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 16384,                          // N
      .block_size = 8,                        // r
      .parallelization = 1,                   // p
      .max_memory_bytes = 512 * 1024 * 1024,  // 512 MB
  };

  auto derived_key =
      ScryptDeriveKey(password, base::as_byte_span(salt_str), scrypt_params);
  ASSERT_TRUE(derived_key.has_value());

  // We use the first 32 bytes for comparison (dkLen=64, but we only need 32).
  std::array<uint8_t, kScryptKeyBytes> expected_key = {
      0x70, 0x23, 0xbd, 0xcb, 0x3a, 0xfd, 0x73, 0x48, 0x46, 0x1c, 0x06,
      0xcd, 0x81, 0xfd, 0x38, 0xeb, 0xfd, 0xa8, 0xfb, 0xba, 0x90, 0x4f,
      0x8e, 0x3e, 0xa9, 0xb5, 0x43, 0xf6, 0x54, 0x5d, 0xa1, 0xf2};

  EXPECT_EQ(*derived_key, expected_key);
}

// https://github.com/jedisct1/libsodium/blob/2909039095d1bf45ad86f647f45f53ecc6b347c9/test/default/secretbox_easy.c#L5
TEST(ScryptUtilsTest, Encrypt_TestVector) {
  std::array<uint8_t, kScryptKeyBytes> key = {
      0x1b, 0x27, 0x55, 0x64, 0x73, 0xe9, 0x85, 0xd4, 0x62, 0xcd, 0x51,
      0x19, 0x7a, 0x9a, 0x46, 0xc7, 0x60, 0x09, 0x54, 0x9e, 0xac, 0x64,
      0x74, 0xf2, 0x06, 0xc4, 0xee, 0x08, 0x44, 0xf6, 0x83, 0x89};

  std::array<uint8_t, kSecretboxNonceSize> nonce = {
      0x69, 0x69, 0x6e, 0xe9, 0x55, 0xb6, 0x2b, 0x73, 0xcd, 0x62, 0xbd, 0xa8,
      0x75, 0xfc, 0x73, 0xd6, 0x82, 0x19, 0xe0, 0x03, 0x6b, 0x7a, 0x0b, 0x37};

  std::vector<uint8_t> plaintext = {
      0xbe, 0x07, 0x5f, 0xc5, 0x3c, 0x81, 0xf2, 0xd5, 0xcf, 0x14, 0x13, 0x16,
      0xeb, 0xeb, 0x0c, 0x7b, 0x52, 0x28, 0xc5, 0x2a, 0x4c, 0x62, 0xcb, 0xd4,
      0x4b, 0x66, 0x84, 0x9b, 0x64, 0x24, 0x4f, 0xfc, 0xe5, 0xec, 0xba, 0xaf,
      0x33, 0xbd, 0x75, 0x1a, 0x1a, 0xc7, 0x28, 0xd4, 0x5e, 0x6c, 0x61, 0x29,
      0x6c, 0xdc, 0x3c, 0x01, 0x23, 0x35, 0x61, 0xf4, 0x1d, 0xb6, 0x6c, 0xce,
      0x31, 0x4a, 0xdb, 0x31, 0x0e, 0x3b, 0xe8, 0x25, 0x0c, 0x46, 0xf0, 0x6d,
      0xce, 0xea, 0x3a, 0x7f, 0xa1, 0x34, 0x80, 0x57, 0xe2, 0xf6, 0x55, 0x6a,
      0xd6, 0xb1, 0x31, 0x8a, 0x02, 0x4a, 0x83, 0x8f, 0x21, 0xaf, 0x1f, 0xde,
      0x04, 0x89, 0x77, 0xeb, 0x48, 0xf5, 0x9f, 0xfd, 0x49, 0x24, 0xca, 0x1c,
      0x60, 0x90, 0x2e, 0x52, 0xf0, 0xa0, 0x89, 0xbc, 0x76, 0x89, 0x70, 0x40,
      0xe0, 0x82, 0xf9, 0x37, 0x76, 0x38, 0x48, 0x64, 0x5e, 0x07, 0x05};

  auto encrypted = ScryptEncrypt(plaintext, key, nonce);
  ASSERT_TRUE(encrypted.has_value());

  std::vector<uint8_t> expected_ciphertext = {
      0xf3, 0xff, 0xc7, 0x70, 0x3f, 0x94, 0x00, 0xe5, 0x2a, 0x7d, 0xfb, 0x4b,
      0x3d, 0x33, 0x05, 0xd9, 0x8e, 0x99, 0x3b, 0x9f, 0x48, 0x68, 0x12, 0x73,
      0xc2, 0x96, 0x50, 0xba, 0x32, 0xfc, 0x76, 0xce, 0x48, 0x33, 0x2e, 0xa7,
      0x16, 0x4d, 0x96, 0xa4, 0x47, 0x6f, 0xb8, 0xc5, 0x31, 0xa1, 0x18, 0x6a,
      0xc0, 0xdf, 0xc1, 0x7c, 0x98, 0xdc, 0xe8, 0x7b, 0x4d, 0xa7, 0xf0, 0x11,
      0xec, 0x48, 0xc9, 0x72, 0x71, 0xd2, 0xc2, 0x0f, 0x9b, 0x92, 0x8f, 0xe2,
      0x27, 0x0d, 0x6f, 0xb8, 0x63, 0xd5, 0x17, 0x38, 0xb4, 0x8e, 0xee, 0xe3,
      0x14, 0xa7, 0xcc, 0x8a, 0xb9, 0x32, 0x16, 0x45, 0x48, 0xe5, 0x26, 0xae,
      0x90, 0x22, 0x43, 0x68, 0x51, 0x7a, 0xcf, 0xea, 0xbd, 0x6b, 0xb3, 0x73,
      0x2b, 0xc0, 0xe9, 0xda, 0x99, 0x83, 0x2b, 0x61, 0xca, 0x01, 0xb6, 0xde,
      0x56, 0x24, 0x4a, 0x9e, 0x88, 0xd5, 0xf9, 0xb3, 0x79, 0x73, 0xf6, 0x22,
      0xa4, 0x3d, 0x14, 0xa6, 0x59, 0x9b, 0x1f, 0x65, 0x4c, 0xb4, 0x5a, 0x74,
      0xe3, 0x55, 0xa5};

  EXPECT_EQ(*encrypted, expected_ciphertext);

  auto decrypted = ScryptDecrypt(*encrypted, nonce, key);
  ASSERT_TRUE(decrypted.has_value());
  EXPECT_EQ(*decrypted, plaintext);
}

}  // namespace brave_wallet
