/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/values.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet {

inline constexpr size_t kEncryptorNonceSize = 12;
inline constexpr size_t kEncryptorSaltSize = 32;
inline const int kPbkdf2Iterations = 310000;
inline const int kPbkdf2KeySize = 256;

// Use password derived key to encrypt/decrypt using AES-256-GCM
class PasswordEncryptor {
 public:
  ~PasswordEncryptor();
  PasswordEncryptor(const PasswordEncryptor&) = delete;
  PasswordEncryptor& operator=(const PasswordEncryptor&) = delete;

  static std::optional<int>& GetPbkdf2IterationsForTesting();
  static base::RepeatingCallback<std::vector<uint8_t>()>&
  GetCreateNonceCallbackForTesting();
  static base::RepeatingCallback<std::vector<uint8_t>()>&
  GetCreateSaltCallbackForTesting();

  static std::vector<uint8_t> CreateNonce();
  static std::vector<uint8_t> CreateSalt();
  static std::unique_ptr<PasswordEncryptor> CreateEncryptor(
      const std::string& password,
      base::span<const uint8_t> salt);

  // With SHA 256 digest
  static std::unique_ptr<PasswordEncryptor> DeriveKeyFromPasswordUsingPbkdf2(
      const std::string& password,
      base::span<const uint8_t> salt,
      size_t iterations,
      size_t key_size_in_bits);

  std::vector<uint8_t> Encrypt(base::span<const uint8_t> plaintext,
                               base::span<const uint8_t> nonce);
  base::Value::Dict EncryptToDict(base::span<const uint8_t> plaintext,
                                  base::span<const uint8_t> nonce);

  std::optional<std::vector<uint8_t>> Decrypt(
      base::span<const uint8_t> ciphertext,
      base::span<const uint8_t> nonce);
  std::optional<std::vector<uint8_t>> DecryptFromDict(
      const base::Value::Dict& encrypted_value);

  // This can only be used by wallet importer
  std::optional<std::vector<uint8_t>> DecryptForImporter(
      base::span<const uint8_t> ciphertext,
      base::span<const uint8_t> nonce);

 private:
  FRIEND_TEST_ALL_PREFIXES(PasswordEncryptorUnitTest, DecryptForImporter);
  explicit PasswordEncryptor(base::span<uint8_t> key);

  // symmetric key used to encrypt and decrypt
  std::vector<uint8_t, crypto::SecureAllocator<uint8_t>> key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PASSWORD_ENCRYPTOR_H_
