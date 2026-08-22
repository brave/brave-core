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

// Scans raw calldata for the setApprovalForAll selector and returns operator
// addresses for each grant (revoke calls are skipped). Catches grants nested
// in opaque wrappers such as multicall. Target-blind, may over-warn.
std::vector<std::string> FindSetApprovalForAllOperatorsByByteScan(
    base::span<const uint8_t> data);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
