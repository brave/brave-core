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
  absl::optional<std::vector<uint8_t>> decrypted =
      aead.Open(ciphertext, nonce, std::vector<uint8_t>());
  if (!decrypted)
    return false;
  *plaintext = std::vector<uint8_t>(decrypted->data(),
                                    decrypted->data() + decrypted->size());
  return true;
}

bool PasswordEncryptor::DecryptForImporter(base::span<const uint8_t> ciphertext,
                                           base::span<const uint8_t> nonce,
                                           std::vector<uint8_t>* plaintext) {
  if (!plaintext)
    return false;
  crypto::Aead aead(crypto::Aead::AES_256_GCM);
  aead.Init(key_);
  // MM uses 16 bytes nonce while boringSSL expect it to be 12
  // https://github.com/MetaMask/browser-passworder/blob/2c8195a4bfe3778571eb35117159f448fef07865/src/index.ts#L42-L51
  //
  // From aead.h in boringSSL
  // Note: AES-GCM should only be used with 12-byte (96-bit) nonces. Although it
  // is specified to take a variable-length nonce, nonces with other lengths are
  // effectively randomized, which means one must consider collisions. Unless
  // implementing an existing protocol which has already specified incorrect
  // parameters, only use 12-byte nonces.
  //
  // so we override the nonce length to prevent DCHECK failure
  if (nonce.size() != aead.NonceLength()) {
    aead.OverrideNonceLength(nonce.size());
  }
  absl::optional<std::vector<uint8_t>> decrypted =
      aead.Open(ciphertext, nonce, std::vector<uint8_t>());
  if (!decrypted)
    return false;
  *plaintext = std::vector<uint8_t>(decrypted->data(),
                                    decrypted->data() + decrypted->size());
  return true;
}

}  // namespace brave_wallet
