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
#include "base/strings/string_number_conversions.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "crypto/hmac.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

namespace {

inline constexpr char kMasterSecret[] = "ed25519 seed";
inline constexpr uint32_t kHardenedOffset = 0x80000000;

// OpenSSL has it's own definition of private key which in fact is key pair.
static_assert(kEd25519KeyPairSize == ED25519_PRIVATE_KEY_LEN);
static_assert(kEd25519PublicKeySize == ED25519_PUBLIC_KEY_LEN);

std::array<uint8_t, kEd25519KeyPairSize> DeriveKeyPairFromPrivateKey(
    base::span<const uint8_t, kEd25519PrivateKeySize> private_key) {
  std::array<uint8_t, kEd25519KeyPairSize> key_pair;
  std::array<uint8_t, kEd25519PublicKeySize> public_key_ignore;
  ED25519_keypair_from_seed(public_key_ignore.data(), key_pair.data(),
                            private_key.data());
  DCHECK_EQ(private_key, base::span(key_pair).first<kEd25519PrivateKeySize>());
  return key_pair;
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
  result->key_pair_ =
      DeriveKeyPairFromPrivateKey(hmac_span.first<kEd25519PrivateKeySize>());
  base::span(result->chain_code_)
      .copy_from(hmac_span.last<kSlip10ChainCodeSize>());
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
  result->key_pair_ = DeriveKeyPairFromPrivateKey(*private_key_fixed_size);
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
  span_writer.WriteU8BigEndian(0);
  span_writer.Write(GetPrivateKeyAsSpan());
  span_writer.WriteU32BigEndian(index + kHardenedOffset);
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
  std::vector<uint8_t> signature(ED25519_SIGNATURE_LEN);
  if (!ED25519_sign(signature.data(), msg.data(), msg.size(),
                    key_pair_.data())) {
    return {};
  }
  return signature;
}

base::span<const uint8_t, kEd25519PrivateKeySize>
HDKeyEd25519::GetPrivateKeyAsSpan() const {
  return base::span(key_pair_).first<kEd25519PrivateKeySize>();
}

base::span<const uint8_t, kEd25519PublicKeySize>
HDKeyEd25519::GetPublicKeyAsSpan() const {
  return base::span(key_pair_).last<kEd25519PublicKeySize>();
}

std::vector<uint8_t> HDKeyEd25519::GetPrivateKeyBytes() const {
  return base::ToVector(GetPrivateKeyAsSpan());
}

std::vector<uint8_t> HDKeyEd25519::GetPublicKeyBytes() const {
  return base::ToVector(GetPublicKeyAsSpan());
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  return EncodeBase58(GetPublicKeyAsSpan());
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  return EncodeBase58(key_pair_);
}

}  // namespace brave_wallet
