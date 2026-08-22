/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/secp256k1_signature.h"

#include <cstdint>
#include <optional>

#include "base/containers/to_vector.h"

namespace brave_wallet {

// static
std::optional<Secp256k1Signature> Secp256k1Signature::CreateFromPayload(
    base::span<const uint8_t, kCompactSignatureSize> rs_bytes,
    uint8_t recid) {
  return CreateFromPayload(rs_bytes.first<kRSize>(), rs_bytes.last<kSSize>(),
                           recid);
}

// static
std::optional<Secp256k1Signature> Secp256k1Signature::CreateFromPayload(
    base::span<const uint8_t, kRSize> r_bytes,
    base::span<const uint8_t, kSSize> s_bytes,
    uint8_t recid) {
  // Valid values for recovery id are [0, 1, 2, 3].
  if (recid > 3) {
    return std::nullopt;
  }
  Secp256k1Signature result;
  base::span(result.bytes_).subspan<0, kRSize>().copy_prefix_from(r_bytes);
  base::span(result.bytes_).subspan<kRSize, kSSize>().copy_prefix_from(s_bytes);
  result.bytes_.back() = recid;
  return result;
}

// static
std::optional<Secp256k1Signature>
Secp256k1Signature::CreateFromRecoverAddressPayload(
    base::span<const uint8_t> bytes) {
  if (bytes.size() != kCompactSignatureSize + 1) {
    return std::nullopt;
  }

  if (bytes.back() < 27) {
    return std::nullopt;
  }

  return CreateFromPayload(bytes.first<kCompactSignatureSize>(), bytes.back());
}

//  static
std::optional<Secp256k1Signature> Secp256k1Signature::CreateFromLedgerVRS(
    bool is_legacy,
    uint256_t chain_id,
    const std::vector<uint8_t>& v,
    const std::vector<uint8_t>& r,
    const std::vector<uint8_t>& s) {}

std::vector<uint8_t> Secp256k1Signature::ToSignatureBytesForEthSignMessage()
    const {
  // MM does not use chain_id for message signing. Just `recovery_id + 27` as
  // last signature byte.
  // https://github.com/MetaMask/eth-sig-util/blob/0832d49b7c2f6d48d22a4496faee3e393081d1ec/src/personal-sign.ts#L43-L44
  // https://github.com/ethereumjs/ethereumjs-monorepo/blob/460368319c57d0bad1683f718a21f557d9e1eec5/packages/util/src/signature.ts#L37-L53
  auto result = base::ToVector(bytes());
  result.back() += 27;
  return result;
}

}  // namespace brave_wallet
