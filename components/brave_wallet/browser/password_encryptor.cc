/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/password_encryptor.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "crypto/aead.h"
#include "crypto/openssl_util.h"
#include "third_party/boringssl/src/include/openssl/evp.h"

namespace brave_wallet {

PasswordEncryptor::PasswordEncryptor(const std::vector<uint8_t> key)
    : key_(key) {}
PasswordEncryptor::~PasswordEncryptor() {
  // zero key
  SecureZeroData(key_.data(), key_.size());
}

// static
std::unique_ptr<PasswordEncryptor>
PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
    const std::string& password,
    base::span<const uint8_t> salt,
    size_t iterations,
    size_t key_size_in_bits) {
  if (key_size_in_bits != 128 && key_size_in_bits != 256)
    return nullptr;

  size_t key_size_in_bytes = key_size_in_bits / 8;

  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
  std::vector<uint8_t> key(key_size_in_bytes);

  int rv = PKCS5_PBKDF2_HMAC(password.data(), password.length(), salt.data(),
                             salt.size(), static_cast<unsigned>(iterations),
                             EVP_sha256(), key_size_in_bytes, key.data());
  std::unique_ptr<PasswordEncryptor> encryptor(new PasswordEncryptor(key));
  return rv == 1 ? std::move(encryptor) : nullptr;
}

bool PasswordEncryptor::Encrypt(base::span<const uint8_t> plaintext,
                                base::span<const uint8_t> nonce,
                                std::vector<uint8_t>* ciphertext) {
  if (!ciphertext)
    return false;
  crypto::Aead aead(crypto::Aead::AES_256_GCM_SIV);
  aead.Init(key_);
  *ciphertext = aead.Seal(plaintext, nonce, std::vector<uint8_t>());
  return true;
}

bool PasswordEncryptor::Decrypt(base::span<const uint8_t> ciphertext,
                                base::span<const uint8_t> nonce,
                                std::vector<uint8_t>* plaintext) {
  if (!plaintext)
    return false;
  crypto::Aead aead(crypto::Aead::AES_256_GCM_SIV);
  aead.Init(key_);
  base::Optional<std::vector<uint8_t>> decrypted =
      aead.Open(ciphertext, nonce, std::vector<uint8_t>());
  if (!decrypted)
    return false;
  *plaintext = std::vector<uint8_t>(decrypted->data(),
                                    decrypted->data() + decrypted->size());
  return true;
}

}  // namespace brave_wallet
