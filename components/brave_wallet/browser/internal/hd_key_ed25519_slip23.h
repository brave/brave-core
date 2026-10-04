/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_SLIP23_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_SLIP23_H_

#include <array>
#include <memory>
#include <string_view>

#include "base/containers/span.h"
#include "base/types/pass_key.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"

namespace brave_wallet {

inline constexpr size_t kSlip23ScalarSize = 32;
inline constexpr size_t kSlip23DerivationScalarSize = 28;
inline constexpr size_t kSlip23PrefixSize = 32;
inline constexpr size_t kSlip23ChainCodeSize = 32;

// This class implements EdDSA over ed25519 functionality as SLIP-0023
// https://github.com/satoshilabs/slips/blob/master/slip-0023.md
class HDKeyEd25519Slip23 {
 public:
  using PassKey = base::PassKey<HDKeyEd25519Slip23>;

  HDKeyEd25519Slip23(
      PassKey,
      base::span<const uint8_t, kSlip23ScalarSize> scalar,
      base::span<const uint8_t, kSlip23PrefixSize> prefix,
      base::span<const uint8_t, kSlip23ChainCodeSize> chain_code,
      base::span<const uint8_t, kEd25519PublicKeySize> public_key);
  HDKeyEd25519Slip23(const HDKeyEd25519Slip23&);
  HDKeyEd25519Slip23& operator=(const HDKeyEd25519Slip23&);
  ~HDKeyEd25519Slip23();

  static std::unique_ptr<HDKeyEd25519Slip23> GenerateMasterKeyFromBip39Entropy(
      base::span<const uint8_t> entropy);

  std::unique_ptr<HDKeyEd25519Slip23> DeriveChild(DerivationIndex index);
  std::unique_ptr<HDKeyEd25519Slip23> DeriveChildFromPath(
      base::span<const DerivationIndex> path);

  std::optional<std::array<uint8_t, kEd25519SignatureSize>> Sign(
      base::span<const uint8_t> msg);

  base::span<const uint8_t, kSlip23ScalarSize> GetScalarAsSpanForTesting() const
      LIFETIME_BOUND;
  base::span<const uint8_t, kSlip23PrefixSize> GetPrefixAsSpanForTesting() const
      LIFETIME_BOUND;
  base::span<const uint8_t, kSlip23ChainCodeSize> GetChainCodeAsSpanForTesting()
      const LIFETIME_BOUND;
  base::span<const uint8_t, kEd25519PublicKeySize> GetPublicKeyAsSpan() const
      LIFETIME_BOUND;

 private:
  static std::unique_ptr<HDKeyEd25519Slip23> FromBip32Entropy(
      base::span<const uint8_t> seed,
      std::string_view hd_path);

  std::array<uint8_t, kSlip23ScalarSize> scalar_ = {};  // k_l
  std::array<uint8_t, kSlip23PrefixSize> prefix_ = {};  // k_r
  std::array<uint8_t, kSlip23ChainCodeSize> chain_code_ = {};
  std::array<uint8_t, kEd25519PublicKeySize> public_key_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_SLIP23_H_
