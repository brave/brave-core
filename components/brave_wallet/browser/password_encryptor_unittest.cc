/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "base/strings/string_piece.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
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
  EXPECT_FALSE(encryptor->Encrypt(ToSpan("bravo"), nonce, nullptr));
  std::vector<uint8_t> ciphertext;
  EXPECT_TRUE(encryptor->Encrypt(ToSpan("bravo"), nonce, &ciphertext));
  EXPECT_FALSE(encryptor->Decrypt(ciphertext, nonce, nullptr));
  std::vector<uint8_t> plaintext;
  EXPECT_TRUE(encryptor->Decrypt(ciphertext, nonce, &plaintext));
  EXPECT_EQ(std::string(plaintext.begin(), plaintext.end()), "bravo");

  // nonce mismatch
  plaintext.clear();
  const std::vector<uint8_t> nonce_ff(12, 0xFF);
  EXPECT_FALSE(encryptor->Decrypt(ciphertext, nonce_ff, &plaintext));

  // empty ciphertext
  plaintext.clear();
  EXPECT_FALSE(encryptor->Decrypt(std::vector<uint8_t>(), nonce, &plaintext));

  // weong ciphertext
  plaintext.clear();
  EXPECT_FALSE(encryptor->Decrypt(ToSpan("wrongcipher"), nonce, &plaintext));

  // password mismatch
  plaintext.clear();
  std::unique_ptr<PasswordEncryptor> encryptor2 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password2", ToSpan("salt"), 100, 256);
  EXPECT_FALSE(encryptor2->Decrypt(ciphertext, nonce, &plaintext));

  // salt mismatch
  plaintext.clear();
  std::unique_ptr<PasswordEncryptor> encryptor3 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt2"), 100, 256);
  EXPECT_FALSE(encryptor3->Decrypt(ciphertext, nonce, &plaintext));

  // iteration mismatch
  plaintext.clear();
  std::unique_ptr<PasswordEncryptor> encryptor4 =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          "password", ToSpan("salt"), 200, 256);
  EXPECT_FALSE(encryptor4->Decrypt(ciphertext, nonce, &plaintext));
}

}  // namespace brave_wallet
