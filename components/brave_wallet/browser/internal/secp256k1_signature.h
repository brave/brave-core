/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_

#include <array>
#include <cstdint>
#include <optional>

#include "base/containers/span.h"

namespace brave_wallet {

inline constexpr size_t kSecp256k1CompactSignatureSize = 64;

class Secp256k1Signature {
 public:
  static std::optional<Secp256k1Signature> CreateFromPayload(
      base::span<const uint8_t, kSecp256k1CompactSignatureSize> rs_bytes,
      uint8_t recid);

  base::span<const uint8_t, kSecp256k1CompactSignatureSize> rs_bytes() const {
    return bytes().first<kSecp256k1CompactSignatureSize>();
  }
  base::span<const uint8_t, kSecp256k1CompactSignatureSize + 1> bytes() const {
    return bytes_;
  }
  uint8_t recid() const { return bytes_.back(); }

 private:
  std::array<uint8_t, kSecp256k1CompactSignatureSize + 1> bytes_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_
