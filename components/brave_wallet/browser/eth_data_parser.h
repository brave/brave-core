/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

std::optional<std::tuple<mojom::TransactionType,    // tx_type
                         std::vector<std::string>,  // tx_params
                         std::vector<std::string>,  // tx_args
                         mojom::SwapInfoPtr>>       // swap_info
GetTransactionInfoFromData(const std::vector<uint8_t>& data);

// Returns the address that ultimately receives the transferred value. Returns
// nullopt for invalid data or other internal errors. The returned address can
// be a ETH address or a Filecoin one.
std::optional<std::string> GetFinalRecipient(
    const std::string& chain_id,
    const std::string& base_to,
    mojom::TransactionType tx_type,
    base::span<const std::string> tx_args);

// Returns true for token transfer and approval types that should be rendered
// generically when they also carry native value, so token-only metadata does
// not hide the native transfer. This is a display policy, not a determination
// of contract payability: ERC-721 permits payable entrypoints, and calldata
// selectors do not encode state mutability.
//
// ETHSend, ETHSwap and ETHFilForwarderTransfer are excluded because their
// classifications already account for native value.
bool IsNonPayableTokenTransactionType(mojom::TransactionType tx_type);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
