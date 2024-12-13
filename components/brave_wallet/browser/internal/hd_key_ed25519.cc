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

}  // namespace

HDKeyEd25519::HDKeyEd25519() = default;
HDKeyEd25519::~HDKeyEd25519() = default;

// Child key derivation constructor.
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#private-parent-key--private-child-key
HDKeyEd25519::HDKeyEd25519(base::span<const uint8_t> key,
                           base::span<const uint8_t> data) {
  auto hmac = HmacSha512(key, data);
  auto scoped_zero_span = ScopedSecureZeroSpan(hmac);
  auto [il, ir] = base::span(hmac).split_at<32>();

  // `public_key` is not used, we use key pair instead.
  std::array<uint8_t, ED25519_PUBLIC_KEY_LEN> public_key;
  ED25519_keypair_from_seed(public_key.data(), key_pair_.AsSpan().data(),
                            il.data());

  chain_code_.AsSpan().copy_from(ir);
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromKeyPair(
    base::span<const uint8_t, kEd25519KeypairSize> key_pair) {
  if (!ValidateKeypair(key_pair)) {
    return nullptr;
  }

  auto result = std::make_unique<HDKeyEd25519>();
  result->key_pair_.AsSpan().copy_from(key_pair);
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
  std::array<uint8_t, kEd25519SignatureSize> signature = {};

  if (!ED25519_sign(signature.data(), msg.data(), msg.size(),
                    key_pair_.AsSpan().data())) {
    return std::nullopt;
  }
  return signature;
}

base::span<const uint8_t, kEd25519SecretKeySize>
HDKeyEd25519::GetPrivateKeyAsSpan() const {
  return key_pair_.AsSpan().first<kEd25519SecretKeySize>();
}

base::span<const uint8_t, kEd25519PublicKeySize>
HDKeyEd25519::GetPublicKeyAsSpan() const {
  return key_pair_.AsSpan().last<kEd25519PublicKeySize>();
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  return EncodeBase58(GetPublicKeyAsSpan());
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  return EncodeBase58(key_pair_.AsSpan());
}

}  // namespace brave_wallet
