/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {
namespace {

constexpr char kMasterSecret[] = "ed25519 seed";

// Validate keypair(private key matches public key) by using first half as a
// seed.
bool ValidateKeypair(base::span<const uint8_t, kEd25519KeypairSize> key_pair) {
  std::array<uint8_t, kEd25519KeypairSize> validated_key_pair;

  std::array<uint8_t, ED25519_PUBLIC_KEY_LEN> public_key;
  ED25519_keypair_from_seed(public_key.data(), validated_key_pair.data(),
                            key_pair.first<ED25519_PRIVATE_KEY_LEN>().data());

  return validated_key_pair == key_pair;
}

// Returns concatenation of (private key, pubic key)
std::array<uint8_t, kEd25519KeypairSize> MakeKeyPair(
    base::span<const uint8_t> private_key,
    base::span<const uint8_t> public_key) {
  std::array<uint8_t, kEd25519KeypairSize> key_pair = {};
  base::span(key_pair).first<kEd25519PrivateKeySize>().copy_from(private_key);
  base::span(key_pair).last<kEd25519PublicKeySize>().copy_from(public_key);
  return key_pair;
}

}  // namespace

HDKeyEd25519::HDKeyEd25519() = default;
HDKeyEd25519::~HDKeyEd25519() = default;

// Child key derivation constructor.
// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
HDKeyEd25519::HDKeyEd25519(base::span<const uint8_t> key,
                           base::span<const uint8_t> data) {
  auto hmac = HmacSha512(key, data);
  auto scoped_zero_span = ScopedSecureZeroSpan(hmac);
  auto [il, ir] = base::span(hmac).split_at<32>();

  private_key_.AsSpan().copy_from(il);
  chain_code_.AsSpan().copy_from(ir);

  // `key_pair` is not used, we need only public key.
  std::array<uint8_t, kEd25519KeypairSize> key_pair;
  ED25519_keypair_from_seed(public_key_.data(), key_pair.data(), il.data());
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromKeyPair(
    base::span<const uint8_t, kEd25519KeypairSize> key_pair) {
  if (!ValidateKeypair(key_pair)) {
    return nullptr;
  }

  auto result = std::make_unique<HDKeyEd25519>();
  result->private_key_.AsSpan().copy_from(
      key_pair.first<kEd25519PrivateKeySize>());
  base::span(result->public_key_)
      .copy_from(key_pair.last<kEd25519PublicKeySize>());
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
  return DeriveChild(kHardenedOffset + index);
}

std::unique_ptr<HDKeyEd25519> HDKeyEd25519::DeriveChild(uint32_t index) {
  CHECK_GE(index, kHardenedOffset);

  std::array<uint8_t, 37> hmac_payload = {};

  auto span_writer = base::SpanWriter(base::span(hmac_payload));
  // https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
  span_writer.Write(base::U8ToBigEndian(0));
  span_writer.Write(GetPrivateKeyAsSpan());
  span_writer.Write(base::U32ToBigEndian(index));
  DCHECK_EQ(span_writer.remaining(), 0u);

  return base::WrapUnique(new HDKeyEd25519(chain_code_.AsSpan(), hmac_payload));
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromSeedAndPath(
    base::span<const uint8_t> seed,
    std::string_view hd_path) {
  auto hd_key = base::WrapUnique(
      new HDKeyEd25519(base::byte_span_from_cstring(kMasterSecret), seed));

  auto nodes = ParseFullHDPath(hd_path);
  if (!nodes) {
    return nullptr;
  }

  for (auto index : *nodes) {
    if (index < kHardenedOffset) {
      return nullptr;
    }
    hd_key = hd_key->DeriveChild(index);
    if (!hd_key) {
      return nullptr;
    }
  }

  return hd_key;
}

std::optional<std::array<uint8_t, kEd25519SignatureSize>> HDKeyEd25519::Sign(
    base::span<const uint8_t> msg) {
  // For the sake of performance `ED25519_sign` does not derive public key from
  // private key, but assumes they are both already available as key_pair.
  std::array<uint8_t, kEd25519KeypairSize> key_pair =
      MakeKeyPair(private_key_.AsSpan(), public_key_);

  std::array<uint8_t, kEd25519SignatureSize> signature = {};
  if (!ED25519_sign(signature.data(), msg.data(), msg.size(),
                    key_pair.data())) {
    return std::nullopt;
  }
  return signature;
}

base::span<const uint8_t, kEd25519PrivateKeySize>
HDKeyEd25519::GetPrivateKeyAsSpan() const {
  return private_key_.AsSpan();
}

base::span<const uint8_t, kEd25519PublicKeySize>
HDKeyEd25519::GetPublicKeyAsSpan() const {
  return public_key_;
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  return EncodeBase58(GetPublicKeyAsSpan());
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  return EncodeBase58(MakeKeyPair(private_key_.AsSpan(), public_key_));
}

}  // namespace brave_wallet
