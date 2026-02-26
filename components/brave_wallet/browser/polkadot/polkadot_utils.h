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
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/scrypt_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// See definition for "path": ["sp_core", "crypto", "AccountId32"]
// https://raw.githubusercontent.com/polkadot-js/api/refs/heads/master/packages/types-support/src/metadata/v16/substrate-types.json
inline constexpr const size_t kPolkadotSubstrateAccountIdSize = 32;

inline constexpr const size_t kPolkadotBlockHashSize = 32;

// SS58 address prefixes by network.
// https://wiki.polkadot.com/learn/learn-account-advanced/
inline constexpr uint16_t kPolkadotPrefix = 0u;  // Polkadot mainnet.
inline constexpr uint16_t kWestendPrefix = 42u;  // Westend testnet.
inline constexpr uint16_t kSubstratePrefix =
    42u;  // Generic Substrate (e.g. export).

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

// Encodes a Polkadot sr25519 private key for export in Polkadot.js JSON format.
// Uses scrypt + xsalsa20-poly1305. Optional salt_for_testing and
// nonce_for_testing allow deterministic output in tests; pass nullptr to use
// random bytes.
std::optional<std::string> EncodePrivateKeyForExport(
    base::span<const uint8_t, kSr25519Pkcs8Size> pkcs8_key,
    std::string_view address,
    std::string_view password,
    const std::array<uint8_t, kScryptSaltSize>* salt_for_testing = nullptr,
    const std::array<uint8_t, kSecretboxNonceSize>* nonce_for_testing =
        nullptr);

// Decodes a JSON-encoded private key export (Polkadot.js format) and returns
// the PKCS#8 secret key. Returns std::nullopt on invalid JSON, wrong password,
// or unsupported encoding.
std::optional<std::array<uint8_t, kSr25519Pkcs8Size>>
DecodePrivateKeyFromExport(std::string_view json_export,
                           std::string_view password);

mojom::uint128Ptr Uint128ToMojom(uint128_t);
uint128_t MojomToUint128(const mojom::uint128Ptr&);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UTILS_H_
