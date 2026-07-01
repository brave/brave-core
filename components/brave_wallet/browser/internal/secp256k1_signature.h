/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class Secp256k1Signature {
 public:
  inline static constexpr size_t kCompactSignatureSize = 64;
  inline static constexpr size_t kRSize = kCompactSignatureSize / 2;
  inline static constexpr size_t kSSize = kCompactSignatureSize / 2;

  static std::optional<Secp256k1Signature> CreateFromPayload(
      base::span<const uint8_t, kCompactSignatureSize> rs_bytes,
      uint8_t recid);
  static std::optional<Secp256k1Signature> CreateFromPayload(
      base::span<const uint8_t, kRSize> r_bytes,
      base::span<const uint8_t, kSSize> s_bytes,
      uint8_t recid);
  static std::optional<Secp256k1Signature> CreateFromRecoverAddressPayload(
      base::span<const uint8_t> bytes);
  static std::optional<Secp256k1Signature> CreateFromHardwareWalletVRS(
      bool is_legacy,
      uint256_t chain_id,
      const std::vector<uint8_t>& v,
      const std::vector<uint8_t>& r,
      const std::vector<uint8_t>& s);

  bool operator==(const Secp256k1Signature& other) const = default;

  base::span<const uint8_t, kCompactSignatureSize> r_bytes() const {
    return rs_bytes().first<kCompactSignatureSize>();
  }

  base::span<const uint8_t, kSSize> s_bytes() const {
    return rs_bytes().last<kSSize>();
  }

  base::span<const uint8_t, kCompactSignatureSize> rs_bytes() const {
    return bytes().first<kCompactSignatureSize>();
  }

  base::span<const uint8_t, kCompactSignatureSize + 1> bytes() const {
    return bytes_;
  }

  uint8_t recid() const { return bytes_.back(); }

  std::vector<uint8_t> ToSignatureBytesForEthSignMessage() const;

 private:
  // Layout is:
  // R(32) | S(32) | RecId(1)
  std::array<uint8_t, kCompactSignatureSize + 1> bytes_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_SECP256K1_SIGNATURE_H_
