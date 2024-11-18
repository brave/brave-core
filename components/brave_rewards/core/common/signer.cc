/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/signer.h"

#include <openssl/evp.h>
#include <openssl/hkdf.h>
#include <utility>

#include "crypto/random.h"
#include "tweetnacl.h"  // NOLINT

namespace {

constexpr uint8_t kHkdfSalt[] = {
    126, 244, 99,  158, 51,  68,  253, 80,  133, 183, 51,  180, 77,
    62,  74,  252, 62,  106, 96,  125, 241, 110, 134, 87,  190, 208,
    158, 84,  125, 69,  246, 207, 162, 247, 107, 172, 37,  34,  53,
    246, 105, 20,  215, 5,   248, 154, 179, 191, 46,  17,  6,   72,
    210, 91,  10,  169, 145, 248, 22,  147, 117, 24,  105, 12};

constexpr int kSeedLength = 32;

constexpr int kSaltLength = 64;

}  // namespace

namespace brave_rewards::internal {

Signer::Signer(std::vector<uint8_t> public_key, std::vector<uint8_t> secret_key)
    : public_key_(std::move(public_key)), secret_key_(std::move(secret_key)) {
  DCHECK_EQ(public_key_.size(),
            static_cast<size_t>(crypto_sign_PUBLICKEYBYTES));
  DCHECK_EQ(secret_key_.size(),
            static_cast<size_t>(crypto_sign_SECRETKEYBYTES));
}

Signer::~Signer() = default;

Signer::Signer(const Signer&) = default;
Signer& Signer::operator=(const Signer&) = default;

std::optional<Signer> Signer::FromRecoverySeed(
    const std::vector<uint8_t>& recovery_seed) {
  if (recovery_seed.size() != kSeedLength) {
    return std::nullopt;
  }

  std::vector<uint8_t> secret_key(kSeedLength);
  const uint8_t info[] = {0};
  int hkdf_res = HKDF(secret_key.data(), kSeedLength, EVP_sha512(),
                      recovery_seed.data(), kSeedLength, kHkdfSalt, kSaltLength,
                      info, sizeof(info) / sizeof(info[0]));

  DCHECK(hkdf_res);

  std::vector<uint8_t> public_key(crypto_sign_PUBLICKEYBYTES);
  secret_key.resize(crypto_sign_SECRETKEYBYTES);

  crypto_sign_keypair(public_key.data(), secret_key.data(), /* seeded */ 1);

  return Signer(std::move(public_key), std::move(secret_key));
}

std::vector<uint8_t> Signer::GenerateRecoverySeed() {
  std::vector<uint8_t> seed(kSeedLength);
  crypto::RandBytes(seed);
  return seed;
}

std::vector<uint8_t> Signer::SignMessage(base::span<const uint8_t> message) {
  size_t max_length = crypto_sign_BYTES + message.size();
  std::vector<uint8_t> signed_message(max_length);

  unsigned long long signed_size = 0;  // NOLINT
  crypto_sign(signed_message.data(), &signed_size, message.data(),
              message.size(), secret_key_.data());

  signed_message.resize(crypto_sign_BYTES);
  return signed_message;
}

}  // namespace brave_rewards::internal
