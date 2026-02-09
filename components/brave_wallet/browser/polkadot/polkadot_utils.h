/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UTILS_H_

#include <stdint.h>

#include <array>
#include <optional>
#include <string>

namespace brave_wallet {

// See definition for "path": ["sp_core", "crypto", "AccountId32"]
// https://raw.githubusercontent.com/polkadot-js/api/refs/heads/master/packages/types-support/src/metadata/v16/substrate-types.json
inline constexpr const size_t kPolkadotSubstrateAccountIdSize = 32;

inline constexpr const size_t kPolkadotBlockHashSize = 32;

// TODO(https://github.com/brave/brave-browser/issues/52054): Eventually
// refactor this class to fail at construction and remove the `std::optional`
// from `ToString()`.
struct PolkadotAddress {
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  std::optional<uint16_t> ss58_prefix;

  std::optional<std::string> ToString() const;

  // TODO(https://github.com/brave/brave-browser/issues/52056): Implement
  // FromString() once we have an infallible ToString() implementation which
  // enables us to reshape the JSON used by the PolkadotTransaction class.
};

// Parse a string provided from the front-end that's intended to be used as a
// destination address for send transactions. The input string can be in ss58
// format or an appropriately sized hex string with the leading "0x".
//
// TODO(https://github.com/brave/brave-browser/issues/51544): Eventually migrate
// off of `const std::string&`.
std::optional<PolkadotAddress> ParsePolkadotAccount(const std::string& input,
                                                    uint16_t ss58_prefix);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UTILS_H_
