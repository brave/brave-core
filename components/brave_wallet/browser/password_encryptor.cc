/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/password_encryptor.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/no_destructor.h"
#include "crypto/aead.h"
#include "crypto/openssl_util.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/evp.h"

namespace brave_wallet {
namespace {
constexpr char kCiphertextKey[] = "ciphertext";
constexpr char kNonceKey[] = "nonce";
}  // namespace

PasswordEncryptor::PasswordEncryptor(base::span<uint8_t> key)
    : key_(key.begin(), key.end()) {}
PasswordEncryptor::~PasswordEncryptor() = default;

// TODO(apaymyshev): Need to use much lesser value for unit tests where this
// value is irrelevant. Otherwise it takes too much time for tests to pass.
// static
std::optional<int>& PasswordEncryptor::GetPbkdf2IterationsForTesting() {
  static std::optional<int> iterations;
  return iterations;
}

// static
base::RepeatingCallback<std::vector<uint8_t>()>&
PasswordEncryptor::GetCreateNonceCallbackForTesting() {
  static base::NoDestructor<base::RepeatingCallback<std::vector<uint8_t>()>>
      callback;
  return *callback.get();
}

// static
base::RepeatingCallback<std::vector<uint8_t>()>&
PasswordEncryptor::GetCreateSaltCallbackForTesting() {
  static base::NoDestructor<base::RepeatingCallback<std::vector<uint8_t>()>>
      callback;
  return *callback.get();
}

// static
std::vector<uint8_t> PasswordEncryptor::CreateNonce() {
  if (GetCreateNonceCallbackForTesting()) {
    return GetCreateNonceCallbackForTesting().Run();  // IN-TEST
  }
  return crypto::RandBytesAsVector(kEncryptorNonceSize);
}

// static
std::vector<uint8_t> PasswordEncryptor::CreateSalt() {
  if (GetCreateSaltCallbackForTesting()) {
    return GetCreateSaltCallbackForTesting().Run();  // IN-TEST
  }
  return crypto::RandBytesAsVector(kEncryptorSaltSize);
}

// static
std::unique_ptr<PasswordEncryptor> PasswordEncryptor::CreateEncryptor(
    const std::string& password,
    base::span<const uint8_t> salt) {
  if (password.empty()) {
    return nullptr;
  }

  if (salt.size() != kEncryptorSaltSize) {
    return nullptr;
  }

  const auto iterations =
      GetPbkdf2IterationsForTesting().value_or(kPbkdf2Iterations);  // IN-TEST

  return PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
      password, salt, iterations, kPbkdf2KeySize);
}

// static
std::unique_ptr<PasswordEncryptor>
PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
    const std::string& password,
    base::span<const uint8_t> salt,
    size_t iterations,
    size_t key_size_in_bits) {
  if (key_size_in_bits != 128 && key_size_in_bits != 256) {
    return nullptr;
  }

  size_t key_size_in_bytes = key_size_in_bits / 8;

  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
  std::vector<uint8_t> key(key_size_in_bytes);

  int rv = PKCS5_PBKDF2_HMAC(password.data(), password.length(), salt.data(),
                             salt.size(), static_cast<unsigned>(iterations),
                             EVP_sha256(), key_size_in_bytes, key.data());
  std::unique_ptr<PasswordEncryptor> encryptor(new PasswordEncryptor(key));
  return rv == 1 ? std::move(encryptor) : nullptr;
}

std::vector<uint8_t> PasswordEncryptor::Encrypt(
    base::span<const uint8_t> plaintext,
    base::span<const uint8_t> nonce) {
  crypto::Aead aead(crypto::Aead::AES_256_GCM_SIV);
  aead.Init(key_);
  return aead.Seal(plaintext, nonce, std::vector<uint8_t>());
}

base::Value::Dict PasswordEncryptor::EncryptToDict(
    base::span<const uint8_t> plaintext,
    base::span<const uint8_t> nonce) {
  base::Value::Dict result;
  result.Set(kCiphertextKey, base::Base64Encode(Encrypt(plaintext, nonce)));
  result.Set(kNonceKey, base::Base64Encode(nonce));
  return result;
}

std::optional<std::vector<uint8_t>> PasswordEncryptor::Decrypt(
    base::span<const uint8_t> ciphertext,
    base::span<const uint8_t> nonce) {
  crypto::Aead aead(crypto::Aead::AES_256_GCM_SIV);
  aead.Init(key_);
  return aead.Open(ciphertext, nonce, std::vector<uint8_t>());
}

std::optional<std::vector<uint8_t>> PasswordEncryptor::DecryptFromDict(
    const base::Value::Dict& encrypted_value) {
  auto* ciphertext_encoded = encrypted_value.FindString(kCiphertextKey);
  if (!ciphertext_encoded) {
    return std::nullopt;
  }
  auto ciphertext = base::Base64Decode(*ciphertext_encoded);
  if (!ciphertext) {
    return std::nullopt;
  }

  auto* nonce_encoded = encrypted_value.FindString(kNonceKey);
  if (!nonce_encoded) {
    return std::nullopt;
  }
  auto nonce = base::Base64Decode(*nonce_encoded);
  if (!nonce) {
    return std::nullopt;
  }

  return Decrypt(*ciphertext, *nonce);
}

std::optional<std::vector<uint8_t>> PasswordEncryptor::DecryptForImporter(
    base::span<const uint8_t> ciphertext,
    base::span<const uint8_t> nonce) {
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
  return aead.Open(ciphertext, nonce, std::vector<uint8_t>());
}

}  // namespace brave_wallet
