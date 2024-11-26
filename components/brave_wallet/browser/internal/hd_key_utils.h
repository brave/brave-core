/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_

#include <optional>
#include <string_view>
#include <vector>

namespace brave_wallet {

inline constexpr char kMasterNode[] = "m";
inline constexpr uint32_t kHardenedOffset = 0x80000000;

// Parses BIP-32 full derivation path into a vector of indexes. Hardened indexes
// expected to end with a single quote per BIP-44 style.
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
// https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
std::optional<std::vector<uint32_t>> ParseFullHDPath(std::string_view path);

}  // namespace brave_wallet
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_UTILS_H_
