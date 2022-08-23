/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// Use password derived key to encrypt/decrypt using AES-256-GCM
class PasswordEncryptor {
 public:
  ~PasswordEncryptor();

  // With SHA 256 digest
  static std::unique_ptr<PasswordEncryptor> DeriveKeyFromPasswordUsingPbkdf2(
      const std::string& password,
      base::span<const uint8_t> salt,
      size_t iterations,
      size_t key_size_in_bits);

  std::vector<uint8_t> Encrypt(base::span<const uint8_t> plaintext,
                               base::span<const uint8_t> nonce);

  absl::optional<std::vector<uint8_t>> Decrypt(
      base::span<const uint8_t> ciphertext,
      base::span<const uint8_t> nonce);

  // This can only be used by wallet importer
  absl::optional<std::vector<uint8_t>> DecryptForImporter(
      base::span<const uint8_t> ciphertext,
      base::span<const uint8_t> nonce);

 private:
  FRIEND_TEST_ALL_PREFIXES(PasswordEncryptorUnitTest, DecryptForImporter);
  explicit PasswordEncryptor(const std::vector<uint8_t> key);

  // symmetric key used to encrypt and decrypt
  std::vector<uint8_t> key_;
  PasswordEncryptor(const PasswordEncryptor&) = delete;
  PasswordEncryptor& operator=(const PasswordEncryptor&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_
