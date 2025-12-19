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

}  // namespace brave_wallet
