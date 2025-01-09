/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_COMMON_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_COMMON_H_

#include <cstdint>
#include <optional>

namespace brave_wallet {

inline constexpr size_t kEd25519SignatureSize = 64;
inline constexpr size_t kEd25519PublicKeySize = 32;
inline constexpr uint32_t kHardenedOffset = 0x80000000;

class DerivationIndex {
 public:
  static DerivationIndex Normal(uint32_t index);
  static DerivationIndex Hardened(uint32_t index);

  bool IsValid() const;

  bool is_hardened() const { return is_hardened_; }

  std::optional<uint32_t> GetValue() const;

 private:
  DerivationIndex(uint32_t index, bool is_hardened);

  uint32_t index_ = 0;
  bool is_hardened_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_COMMON_H_
