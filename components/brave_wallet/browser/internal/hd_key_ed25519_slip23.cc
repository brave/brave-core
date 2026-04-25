/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519_slip23.h"

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "crypto/hmac.h"
#include "crypto/kdf.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

namespace {

// https://github.com/cardano-foundation/CIPs/blob/master/CIP-0003/Icarus.md
inline constexpr uint32_t kCardanoIcarusMasterIterations = 4096;

// https://datatracker.ietf.org/doc/html/rfc8032#section-5.1.5
// requires scalar to follow this requirements 'The lowest 3 bits of the first
// octet are cleared, the highest bit of the last octet is cleared, and the
// second highest bit of the last octet is set'.
// https://input-output-hk.github.io/adrestia/static/Ed25519_BIP.pdf extends
// this requirement to `We admit only those k such that the third highest bit
// of the last byte of k is zero`
base::span<uint8_t, kSlip23ScalarSize> ClampScalarEd25519Bip32(
    base::span<uint8_t, kSlip23ScalarSize> scalar) {
  scalar[0] &= 0b1111'1000;  // The lowest 3 bits of the first
                             // octet are cleared

  scalar[31] &= 0b0111'1111;  // highest bit of the last octet is cleared and
  scalar[31] &= 0b1101'1111;  // third highest bit of the last byte of k is zero

  scalar[31] |= 0b0100'0000;  // the second highest bit of the last octet is set
  return scalar;
}

bool IsValidEd25519Scalar(base::span<const uint8_t, kSlip23ScalarSize> scalar) {
  return ED25519_is_scalar_pruned(scalar.data());
}

std::optional<std::array<uint8_t, 32>> PubkeyFromScalar(
    base::span<const uint8_t, 32> scalar) {
  DCHECK(IsValidEd25519Scalar(scalar));
  std::array<uint8_t, 32> public_key;
  if (!ED25519_pubkey_from_scalar(public_key.data(), scalar.data())) {
    return std::nullopt;
  }
  return public_key;
}

std::array<uint8_t, kSlip23ScalarSize> CalculateDerivedScalar(
    base::span<const uint8_t, kSlip23ScalarSize> parent_scalar,
    base::span<const uint8_t, kSlip23DerivationScalarSize> zl) {
  std::array<uint8_t, kSlip23ScalarSize> out = {};

  uint8_t carry = 0;
  for (auto i = 0u; i < kSlip23DerivationScalarSize; i++) {
    uint32_t r = parent_scalar[i] + (zl[i] << 3) + carry;
    out[i] = r & 0xff;
    carry = r >> 8;
  }
  for (auto i = kSlip23DerivationScalarSize; i < kSlip23ScalarSize; i++) {
    uint32_t r = parent_scalar[i] + carry;
    out[i] = r & 0xff;
    carry = r >> 8;
  }
  return out;
}

std::array<uint8_t, kSlip23PrefixSize> CalculateDerivedPrefix(
    base::span<const uint8_t, kSlip23PrefixSize> parent_prefix,
    base::span<const uint8_t, kSlip23PrefixSize> zr) {
  std::array<uint8_t, kSlip23PrefixSize> out = {};

  uint8_t carry = 0;
  for (auto i = 0u; i < kSlip23PrefixSize; i++) {
    uint32_t r = parent_prefix[i] + zr[i] + carry;
    out[i] = r;
    carry = r >> 8;
  }

  return out;
}

std::array<uint8_t, kSlip23ChainCodeSize> CalculateDerivedChainCode(
    base::span<const uint8_t, crypto::hash::kSha512Size> z_hmac) {
  std::array<uint8_t, kSlip23ChainCodeSize> chain_code;
  base::span(chain_code)
      .copy_from(base::span(z_hmac).last<kSlip23ChainCodeSize>());
  return chain_code;
}

}  // namespace

HDKeyEd25519Slip23::~HDKeyEd25519Slip23() = default;
HDKeyEd25519Slip23::HDKeyEd25519Slip23(const HDKeyEd25519Slip23&) = default;
HDKeyEd25519Slip23& HDKeyEd25519Slip23::operator=(const HDKeyEd25519Slip23&) =
    default;

// Child key derivation constructor.
HDKeyEd25519Slip23::HDKeyEd25519Slip23(
    PassKey,
    base::span<const uint8_t, kSlip23ScalarSize> scalar,
    base::span<const uint8_t, kSlip23PrefixSize> prefix,
    base::span<const uint8_t, kSlip23ChainCodeSize> chain_code,
    base::span<const uint8_t, kEd25519PublicKeySize> public_key) {
  base::span(scalar_).copy_from(scalar);
  base::span(prefix_).copy_from(prefix);
  base::span(chain_code_).copy_from(chain_code);
  base::span(public_key_).copy_from(public_key);
}

std::unique_ptr<HDKeyEd25519Slip23> HDKeyEd25519Slip23::DeriveChild(
    DerivationIndex index) {
  auto raw_index_value = index.GetValue();
  if (!raw_index_value) {
    return nullptr;
  }

  std::array<uint8_t, crypto::hash::kSha512Size> z_hmac = {};
  std::array<uint8_t, crypto::hash::kSha512Size> cc_hmac = {};

  if (index.is_hardened()) {
    std::array<uint8_t, 1 + 32 + 32 + 4> data;
    auto span_writer = base::SpanWriter(base::span(data));
    span_writer.Skip(1u);
    span_writer.Write(scalar_);
    span_writer.Write(prefix_);
    span_writer.WriteU32LittleEndian(*raw_index_value);
    DCHECK_EQ(span_writer.remaining(), 0u);

    data[0] = 0x00;
    z_hmac = crypto::hmac::SignSha512(chain_code_, data);
    data[0] = 0x01;
    cc_hmac = crypto::hmac::SignSha512(chain_code_, data);
  } else {
    std::array<uint8_t, 1 + 32 + 4> data;
    auto span_writer = base::SpanWriter(base::span(data));
    span_writer.Skip(1u);
    span_writer.Write(public_key_);
    span_writer.WriteU32LittleEndian(*raw_index_value);
    DCHECK_EQ(span_writer.remaining(), 0u);

    data[0] = 0x02;
    z_hmac = crypto::hmac::SignSha512(chain_code_, data);
    data[0] = 0x03;
    cc_hmac = crypto::hmac::SignSha512(chain_code_, data);
  }

  auto derived_scalar = CalculateDerivedScalar(
      scalar_, base::span(z_hmac).first<kSlip23DerivationScalarSize>());

  auto pubkey = PubkeyFromScalar(derived_scalar);
  if (!pubkey) {
    return nullptr;
  }

  return std::make_unique<HDKeyEd25519Slip23>(
      PassKey(), derived_scalar,
      CalculateDerivedPrefix(prefix_,
                             base::span(z_hmac).last<kSlip23PrefixSize>()),
      CalculateDerivedChainCode(cc_hmac), *pubkey);
}

std::unique_ptr<HDKeyEd25519Slip23> HDKeyEd25519Slip23::DeriveChildFromPath(
    base::span<const DerivationIndex> path) {
  auto hd_key = std::make_unique<HDKeyEd25519Slip23>(*this);

  for (auto index : path) {
    hd_key = hd_key->DeriveChild(index);
    if (!hd_key) {
      return nullptr;
    }
  }

  return hd_key;
}

// static
std::unique_ptr<HDKeyEd25519Slip23>
HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(
    base::span<const uint8_t> entropy) {
  // https://github.com/cardano-foundation/CIPs/blob/master/CIP-0003/Icarus.md
  std::array<uint8_t,
             kSlip23ScalarSize + kSlip23PrefixSize + kSlip23ChainCodeSize>
      xprv;
  std::string passphrase;

  if (!crypto::kdf::DeriveKeyPbkdf2HmacSha512(
          {.iterations = kCardanoIcarusMasterIterations}, {},
          base::as_byte_span(entropy), xprv)) {
    return nullptr;
  }

  auto xprv_span = base::span(xprv);
  auto scalar = ClampScalarEd25519Bip32(xprv_span.first<kSlip23ScalarSize>());
  auto pubkey = PubkeyFromScalar(scalar);
  if (!pubkey) {
    return nullptr;
  }

  return std::make_unique<HDKeyEd25519Slip23>(
      PassKey(), scalar,
      xprv_span.subspan<kSlip23ScalarSize, kSlip23PrefixSize>(),
      xprv_span.last<kSlip23ChainCodeSize>(), *pubkey);
}

std::optional<std::array<uint8_t, kEd25519SignatureSize>>
HDKeyEd25519Slip23::Sign(base::span<const uint8_t> msg) {
  if (!IsValidEd25519Scalar(scalar_)) {
    return std::nullopt;
  }

  std::array<uint8_t, kEd25519SignatureSize> signature = {};

  if (!ED25519_sign_with_scalar_and_prefix(
          signature.data(), msg.data(), msg.size(), scalar_.data(),
          prefix_.data(), public_key_.data())) {
    return std::nullopt;
  }
  return signature;
}

base::span<const uint8_t, kSlip23ScalarSize>
HDKeyEd25519Slip23::GetScalarAsSpanForTesting() const {
  return base::span(scalar_);
}

base::span<const uint8_t, kSlip23PrefixSize>
HDKeyEd25519Slip23::GetPrefixAsSpanForTesting() const {
  return base::span(prefix_);
}

base::span<const uint8_t, kSlip23ChainCodeSize>
HDKeyEd25519Slip23::GetChainCodeAsSpanForTesting() const {
  return base::span(chain_code_);
}

base::span<const uint8_t, kEd25519PublicKeySize>
HDKeyEd25519Slip23::GetPublicKeyAsSpan() const {
  return base::span(public_key_);
}

}  // namespace brave_wallet
