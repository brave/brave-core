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

// Validate keypair by signing and verifying signature to ensure included
// private key matches public key.
bool ValidateKeypair(base::span<const uint8_t, kEd25519KeypairSize> key_pair) {
  std::array<uint8_t, kEd25519SignatureSize> signature = {};
  auto msg = base::byte_span_from_cstring("brave");

  CHECK(
      ED25519_sign(signature.data(), msg.data(), msg.size(), key_pair.data()));

  return !!ED25519_verify(msg.data(), msg.size(), signature.data(),
                          key_pair.last<kEd25519PublicKeySize>().data());
}

}  // namespace

HDKeyEd25519::HDKeyEd25519() = default;
HDKeyEd25519::~HDKeyEd25519() = default;

// Child key derivation constructor.
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#private-parent-key--private-child-key
HDKeyEd25519::HDKeyEd25519(base::span<const uint8_t> key,
                           base::span<const uint8_t> data) {
  auto hmac = HmacSha512(key, data);
  auto [il, ir] = base::span(hmac).split_at<32>();

  // `public_key` is not used, we use key pair instead.
  std::array<uint8_t, ED25519_PUBLIC_KEY_LEN> public_key;
  ED25519_keypair_from_seed(public_key.data(), key_pair_.data(), il.data());

  base::span(chain_code_).copy_from(ir);
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromKeyPair(
    base::span<const uint8_t, kEd25519KeypairSize> key_pair) {
  if (!ValidateKeypair(key_pair)) {
    return nullptr;
  }

  auto result = std::make_unique<HDKeyEd25519>();
  base::span(result->key_pair_).copy_from(key_pair);
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
  std::vector<uint8_t> hmac_payload(37);

  auto span_writer = base::SpanWriter(base::span(hmac_payload));
  // https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
  span_writer.Write(base::U8ToBigEndian(0));
  span_writer.Write(GetPrivateKeyAsSpan());
  span_writer.Write(base::U32ToBigEndian(index));

  return base::WrapUnique(new HDKeyEd25519(chain_code_, hmac_payload));
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

std::array<uint8_t, kEd25519SignatureSize> HDKeyEd25519::Sign(
    base::span<const uint8_t> msg) {
  std::array<uint8_t, kEd25519SignatureSize> signature = {};

  CHECK(
      ED25519_sign(signature.data(), msg.data(), msg.size(), key_pair_.data()));
  return signature;
}

base::span<const uint8_t, kEd25519SecretKeySize>
HDKeyEd25519::GetPrivateKeyAsSpan() const {
  return base::span(key_pair_).first<kEd25519SecretKeySize>();
}

base::span<const uint8_t, kEd25519PublicKeySize>
HDKeyEd25519::GetPublicKeyAsSpan() const {
  return base::span(key_pair_).last<kEd25519PublicKeySize>();
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  return EncodeBase58(GetPublicKeyAsSpan());
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  return EncodeBase58(key_pair_);
}

}  // namespace brave_wallet
