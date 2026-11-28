/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_ADDRESS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_ADDRESS_UTILS_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/mojom/brave_wallet_common.mojom-forward.h"

namespace brave_wallet {

// ENSIP-9 coin type for Ethereum mainnet.
inline constexpr uint32_t kEnsEthCoinType = 60;

// ENS coin type for `coin` on `chain_id`: 60 for Ethereum mainnet and its L1
// testnets, ENSIP-11 (0x80000000 | chainId) for other EVM chains, SLIP-44 for
// non-EVM mainnets.
// nullopt for non-EVM testnets and chain IDs ENS can't represent.
std::optional<uint32_t> GetEnsCoinType(mojom::CoinType coin,
                                       std::string_view chain_id);

// Converts an ENSIP-9 binary address record to the wallet's address string.
// nullopt if the record is malformed or isn't a type the wallet can send to.
std::optional<std::string> EnsAddrRecordToAddress(
    mojom::CoinType coin,
    base::span<const uint8_t> record);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ENS_ADDRESS_UTILS_H_
