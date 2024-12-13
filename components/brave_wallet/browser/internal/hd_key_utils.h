/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_

#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

namespace internal {
void SecureZeroBuffer(base::span<uint8_t> data);
}

inline constexpr char kMasterNode[] = "m";
inline constexpr uint32_t kHardenedOffset = 0x80000000;

// Parses BIP-32 full derivation path into a vector of indexes. Hardened indexes
// expected to end with a single quote per BIP-44 style.
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
// https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
std::optional<std::vector<uint32_t>> ParseFullHDPath(std::string_view path);

class ScopedSecureZeroSpan {
 public:
  explicit ScopedSecureZeroSpan(base::span<uint8_t> span);
  ~ScopedSecureZeroSpan();

 private:
  base::span<uint8_t> span_;
};

template <size_t N>
class SecureByteArray {
 public:
  SecureByteArray() = default;
  ~SecureByteArray() { internal::SecureZeroBuffer(AsSpan()); }

  base::span<uint8_t, N> AsSpan() { return data_; }
  base::span<const uint8_t, N> AsSpan() const { return data_; }

 private:
  std::array<uint8_t, N> data_ = {};
};

}  // namespace brave_wallet
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_
