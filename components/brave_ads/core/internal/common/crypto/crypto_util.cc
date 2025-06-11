/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"

#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cstdint>
#include <iterator>

#include "base/base64.h"
#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/crypto/key_pair_info.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "tweetnacl.h"  // NOLINT

namespace brave_ads::crypto {

namespace {

constexpr size_t kHKDFSeedLength = 32;

constexpr uint8_t kHKDFSalt[] = {
    126, 244, 99,  158, 51,  68,  253, 80,  133, 183, 51,  180, 77,
    62,  74,  252, 62,  106, 96,  125, 241, 110, 134, 87,  190, 208,
    158, 84,  125, 69,  246, 207, 162, 247, 107, 172, 37,  34,  53,
    246, 105, 20,  215, 5,   248, 154, 179, 191, 46,  17,  6,   72,
    210, 91,  10,  169, 145, 248, 22,  147, 117, 24,  105, 12};

std::optional<std::vector<uint8_t>> GetHKDF(
    const std::vector<uint8_t>& secret) {
  CHECK(!secret.empty());

  std::vector<uint8_t> derived_key(kHKDFSeedLength);

  constexpr uint8_t kInfo[] = {0};

  if (HKDF(derived_key.data(), kHKDFSeedLength, EVP_sha512(), secret.data(),
           secret.size(), kHKDFSalt, std::size(kHKDFSalt), kInfo,
           sizeof(kInfo) / sizeof(kInfo[0])) == 0) {
    return std::nullopt;
  }

  return derived_key;
}

KeyPairInfo GenerateSignKeyPairFromSecret(const std::vector<uint8_t>& secret) {
  CHECK(!secret.empty());

  std::vector<uint8_t> secret_key = secret;
  secret_key.resize(crypto_sign_SECRETKEYBYTES);
  std::vector<uint8_t> public_key(crypto_sign_PUBLICKEYBYTES);
  crypto_sign_keypair(public_key.data(), secret_key.data(), /*seeded=*/1);

  KeyPairInfo key_pair;
  key_pair.public_key = public_key;
  key_pair.secret_key = secret_key;

  return key_pair;
}

}  // namespace

std::vector<uint8_t> Sha256(const std::string& value) {
  std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
  SHA256(reinterpret_cast<const uint8_t*>(value.data()), value.length(),
         hash.data());
  return hash;
}

std::optional<KeyPairInfo> GenerateSignKeyPairFromSeed(
    const std::vector<uint8_t>& seed) {
  if (seed.empty()) {
    return std::nullopt;
  }

  std::optional<std::vector<uint8_t>> derived_key = GetHKDF(seed);
  if (!derived_key) {
    return std::nullopt;
  }

  return GenerateSignKeyPairFromSecret(*derived_key);
}

KeyPairInfo GenerateBoxKeyPair() {
  std::vector<uint8_t> public_key(crypto_box_PUBLICKEYBYTES);
  std::vector<uint8_t> secret_key(crypto_box_SECRETKEYBYTES);
  crypto_box_keypair(public_key.data(), secret_key.data());

  KeyPairInfo key_pair;
  key_pair.public_key = public_key;
  key_pair.secret_key = secret_key;

  return key_pair;
}

std::vector<uint8_t> GenerateRandomNonce() {
  return ::crypto::RandBytesAsVector(crypto_box_NONCEBYTES);
}

std::optional<std::string> Sign(const std::string& message,
                                const std::string& secret_key_base64) {
  std::optional<std::vector<uint8_t>> secret_key =
      base::Base64Decode(secret_key_base64);
  if (!secret_key) {
    return std::nullopt;
  }

  const std::vector<uint8_t> message_bytes(message.cbegin(), message.cend());

  std::vector<uint8_t> signature;
  signature.resize(ED25519_SIGNATURE_LEN);
  if (ED25519_sign(signature.data(), message_bytes.data(), message_bytes.size(),
                   secret_key->data()) == 0) {
    return std::nullopt;
  }

  return base::Base64Encode(signature);
}

bool Verify(const std::string& message,
            const std::string& public_key_base64,
            const std::string& signature_base64) {
  std::optional<std::vector<uint8_t>> public_key =
      base::Base64Decode(public_key_base64);
  if (!public_key) {
    return false;
  }

  std::optional<std::vector<uint8_t>> signature =
      base::Base64Decode(signature_base64);
  if (!signature) {
    return false;
  }

  const std::vector<uint8_t> message_bytes(message.cbegin(), message.cend());

  return ED25519_verify(message_bytes.data(), message_bytes.size(),
                        signature->data(), public_key->data()) != 0;
}

std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plaintext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& public_key,
                             const std::vector<uint8_t>& secret_key) {
  std::vector<uint8_t> padded_plaintext = plaintext;
  padded_plaintext.insert(padded_plaintext.cbegin(), crypto_box_ZEROBYTES, 0);

  std::vector<uint8_t> ciphertext(padded_plaintext.size());
  crypto_box(ciphertext.data(), padded_plaintext.data(),
             padded_plaintext.size(), nonce.data(), public_key.data(),
             secret_key.data());

  return ciphertext;
}

std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& ciphertext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& public_key,
                             const std::vector<uint8_t>& secret_key) {
  std::vector<uint8_t> padded_plaintext(ciphertext.size());
  crypto_box_open(padded_plaintext.data(), ciphertext.data(), ciphertext.size(),
                  nonce.data(), public_key.data(), secret_key.data());

  std::vector<uint8_t> plaintext;
  std::ranges::copy_n(padded_plaintext.cbegin() + crypto_box_ZEROBYTES,
                      padded_plaintext.size() - crypto_box_ZEROBYTES,
                      std::back_inserter(plaintext));
  return plaintext;
}

}  // namespace brave_ads::crypto
