/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BIP39_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BIP39_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

// Utility functions for BIP39 mnemonics support.
// https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki
namespace brave_wallet::bip39 {

inline constexpr uint32_t kDefaultEntropySize = 16;
inline constexpr uint32_t kLegacyEthEntropySize = 32;

// Generate mnemonic from entropy bytes following BIP39.
// If |entropy.size()| is not in 16, 20, 24, 28, 32 range or
// allocation failure, the std::nullopt will be returned.
std::optional<std::string> GenerateMnemonic(base::span<const uint8_t> entropy);

// Generate 64 bytes seed from mnemonic following BIP39.
std::optional<std::vector<uint8_t>> MnemonicToSeed(
    std::string_view mnemonic,
    std::string_view passphrase = "");

// This is mainly used for restoring legacy brave crypto wallet
std::optional<std::vector<uint8_t>> MnemonicToEntropy(
    std::string_view mnemonic);

bool IsValidMnemonic(std::string_view mnemonic);

}  // namespace brave_wallet::bip39

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BIP39_H_
