/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "crypto/hmac.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {
namespace {

inline constexpr char kMasterSecret[] = "ed25519 seed";
inline constexpr uint32_t kHardenedOffset = 0x80000000;

// Returns concatenation of (private key, pubic key)
std::array<uint8_t, ED25519_PRIVATE_KEY_LEN> MakeKeyPair(
    base::span<const uint8_t> private_key,
    base::span<const uint8_t> public_key) {
  std::array<uint8_t, ED25519_PRIVATE_KEY_LEN> key_pair = {};
  base::span(key_pair).first<kEd25519PrivateKeySize>().copy_from(private_key);
  base::span(key_pair).last<kEd25519PublicKeySize>().copy_from(public_key);
  return key_pair;
}

std::array<uint8_t, kEd25519PublicKeySize> DerivePubkeyFromPrivateKey(
    base::span<const uint8_t> private_key) {
  std::array<uint8_t, kEd25519PublicKeySize> public_key = {};

  // `key_pair` is not used and discarded, we need only public key.
  std::array<uint8_t, ED25519_PRIVATE_KEY_LEN> key_pair;
  ED25519_keypair_from_seed(public_key.data(), key_pair.data(),
                            private_key.data());

  return public_key;
}

}  // namespace

HDKeyEd25519::HDKeyEd25519() = default;
HDKeyEd25519::~HDKeyEd25519() = default;

// Child key derivation.
// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::DeriveFromHmacPayload(
    base::span<const uint8_t> key,
    base::span<const uint8_t> data) {
  auto hmac = crypto::hmac::SignSha512(key, data);
  auto hmac_span = base::span(hmac);

  auto result = std::make_unique<HDKeyEd25519>();
  base::span(result->private_key_)
      .copy_from(hmac_span.first<kEd25519PrivateKeySize>());
  base::span(result->chain_code_)
      .copy_from(hmac_span.last<kSlip10ChainCodeSize>());
  result->public_key_ = DerivePubkeyFromPrivateKey(result->private_key_);
  return result;
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromPrivateKey(
    base::span<const uint8_t> private_key) {
  auto private_key_fixed_size =
      private_key.to_fixed_extent<kEd25519PrivateKeySize>();
  if (!private_key_fixed_size) {
    return nullptr;
  }

  auto result = std::make_unique<HDKeyEd25519>();
  base::span(result->private_key_).copy_from(*private_key_fixed_size);
  result->public_key_ = DerivePubkeyFromPrivateKey(result->private_key_);
  return result;
}

// index should be 0 to 2^31-1
// If anything failed, nullptr will be returned
// Normal derivation is not supported for ed25519.
// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::DeriveHardenedChild(
    uint32_t index) {
  if (index >= kHardenedOffset) {
    return nullptr;
  }
  std::array<uint8_t, 37> hmac_payload = {};

  auto span_writer = base::SpanWriter(base::span(hmac_payload));
  // https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
  span_writer.Write(base::U8ToBigEndian(0));
  span_writer.Write(private_key_);
  span_writer.Write(base::U32ToBigEndian(index + kHardenedOffset));
  DCHECK_EQ(span_writer.remaining(), 0u);

  return DeriveFromHmacPayload(chain_code_, hmac_payload);
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  return DeriveFromHmacPayload(base::byte_span_from_cstring(kMasterSecret),
                               seed);
}

std::vector<uint8_t> HDKeyEd25519::Sign(base::span<const uint8_t> msg) {
  // For the sake of performance `ED25519_sign` does not derive public key from
  // private key, but assumes they are both already available as key_pair.
  std::array<uint8_t, ED25519_PRIVATE_KEY_LEN> key_pair =
      MakeKeyPair(private_key_, public_key_);

  std::vector<uint8_t> signature(ED25519_SIGNATURE_LEN);
  if (!ED25519_sign(signature.data(), msg.data(), msg.size(),
                    key_pair.data())) {
    return {};
  }
  return signature;
}

std::vector<uint8_t> HDKeyEd25519::GetPrivateKeyBytes() const {
  return base::ToVector(private_key_);
}

std::vector<uint8_t> HDKeyEd25519::GetPublicKeyBytes() const {
  return base::ToVector(public_key_);
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  return EncodeBase58(public_key_);
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  return EncodeBase58(MakeKeyPair(private_key_, public_key_));
}

}  // namespace brave_wallet
