/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/secp256k1_signature.h"

#include <cstdint>
#include <optional>

namespace brave_wallet {

// static
std::optional<Secp256k1Signature> Secp256k1Signature::CreateFromPayload(
    base::span<const uint8_t, kSecp256k1CompactSignatureSize> rs_bytes,
    uint8_t recid) {
  // Valid values for recovery id are [0, 1, 2, 3].
  if (recid > 3) {
    return std::nullopt;
  }
  Secp256k1Signature result;
  base::span(result.bytes_).copy_prefix_from(rs_bytes);
  result.bytes_.back() = recid;
  return result;
}

}  // namespace brave_wallet
