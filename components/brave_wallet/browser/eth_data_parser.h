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
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

std::optional<std::tuple<mojom::TransactionType,    // tx_type
                         std::vector<std::string>,  // tx_params
                         std::vector<std::string>,  // tx_args
                         mojom::SwapInfoPtr>>       // swap_info
GetTransactionInfoFromData(const std::vector<uint8_t>& data);

// An ERC-20 `Approval` or ERC-721/1155 `ApprovalForAll` grant found in
// simulated event logs.
struct AuthorizationFinding {
  enum class Kind { kErc20Approval, kApprovalForAll };
  Kind kind;
  std::string token_contract;  // log.address
  std::string grantor;         // topics[1] (owner)
  std::string grantee;         // topics[2] (spender/operator)
  std::string raw_value;       // log.data
};

// Scans simulated calls for authorization events. Reverted calls are skipped.
std::vector<AuthorizationFinding> ScanAuthorizations(
    const std::vector<SimulatedCall>& calls);

// Offline fallback: scans raw calldata for the setApprovalForAll selector and
// returns operator addresses for each grant. Target-blind, may over-warn.
std::vector<std::string> FindSetApprovalForAllOperatorsByByteScan(
    base::span<const uint8_t> data);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_DATA_PARSER_H_
