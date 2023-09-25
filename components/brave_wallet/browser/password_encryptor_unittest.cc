/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "crypto/aead.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
base::span<const uint8_t> ToSpan(std::string_view sv) {
  return base::as_bytes(base::make_span(sv));
}
std::string ToString(const std::vector<uint8_t>& v) {
  return std::string(v.begin(), v.end());
}
}  // namespace

TEST(PasswordEncryptorUnitTest, DeriveKeyFromPasswordUsingPbkdf2) {
  EXPECT_EQ(PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
                "password", ToSpan("salt"), 100, 64),
            nullptr);
  EXPECT_NE(PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
                "password", ToSpan("salt"), 100, 128),
            nullptr);
  EXPECT_NE(PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
                "password", ToSpan("salt"), 100, 256),
            nullptr);
  EXPECT_EQ(PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
                "password", ToSpan("salt"), 0, 256),
            nullptr);
}

TEST(PasswordEncryptorUnitTest, EncryptAndDecrypt) {
  std::unique_ptr<PasswordEncryptor> encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt"), 100, 256);
  const std::vector<uint8_t> nonce(12, 0xAB);
  auto ciphertext = encryptor->Encrypt(ToSpan("bravo"), nonce);
  EXPECT_EQ("bravo", ToString(*encryptor->Decrypt(ciphertext, nonce)));

  // nonce mismatch
  const std::vector<uint8_t> nonce_ff(12, 0xFF);
  EXPECT_FALSE(encryptor->Decrypt(ciphertext, nonce_ff));

  // empty ciphertext
  EXPECT_FALSE(encryptor->Decrypt(std::vector<uint8_t>(), nonce));

  // wrong ciphertext
  EXPECT_FALSE(encryptor->Decrypt(ToSpan("wrongcipher"), nonce));

  // password mismatch
  std::unique_ptr<PasswordEncryptor> encryptor2 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password2", ToSpan("salt"), 100, 256);
  EXPECT_FALSE(encryptor2->Decrypt(ciphertext, nonce));

  // salt mismatch
  std::unique_ptr<PasswordEncryptor> encryptor3 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt2"), 100, 256);
  EXPECT_FALSE(encryptor3->Decrypt(ciphertext, nonce));

  // iteration mismatch
  std::unique_ptr<PasswordEncryptor> encryptor4 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt"), 200, 256);
  EXPECT_FALSE(encryptor4->Decrypt(ciphertext, nonce));
}

TEST(PasswordEncryptorUnitTest, DecryptForImporter) {
  std::unique_ptr<PasswordEncryptor> encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt"), 100, 256);
  const std::vector<uint8_t> nonce_12(12, 0xAB);
  const std::vector<uint8_t> nonce_16(16, 0xAB);

  crypto::Aead aead(crypto::Aead::AES_256_GCM);
  aead.Init(encryptor->key_);
  std::vector<uint8_t> ciphertext;
  ciphertext =
      aead.Seal(ToSpan("importer12"), nonce_12, std::vector<uint8_t>());
  auto plaintext = encryptor->DecryptForImporter(ciphertext, nonce_12);
  ASSERT_TRUE(plaintext);
  EXPECT_EQ(ToString(*plaintext), "importer12");

  aead.OverrideNonceLength(16);
  ciphertext =
      aead.Seal(ToSpan("importer16"), nonce_16, std::vector<uint8_t>());
  plaintext = encryptor->DecryptForImporter(ciphertext, nonce_16);
  ASSERT_TRUE(plaintext);
  EXPECT_EQ(ToString(*plaintext), "importer16");
}

}  // namespace brave_wallet
