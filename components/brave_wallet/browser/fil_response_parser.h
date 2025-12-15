/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

struct ParseFilEstimateGasResult {
  std::string gas_premium;
  std::string gas_fee_cap;
  int64_t gas_limit = 0;
};

// Returns the balance of the account of given address.
std::optional<std::string> ParseFilGetBalance(const base::Value& json_value);
// Returns the transaction count of given address.
std::optional<uint64_t> ParseFilGetTransactionCount(
    const base::Value& json_value);
// Returns Gas estimation values.
std::optional<ParseFilEstimateGasResult> ParseFilEstimateGas(
    const base::Value& json_value);
// Returns parsed chain head CID.
std::optional<uint64_t> ParseFilGetChainHead(const base::Value& json_value);
// Returns parsed receipt exit code.
std::optional<int64_t> ParseFilStateSearchMsgLimited(
    const base::Value& json_value,
    const std::string& cid);
// Returns parsed transaction CID.
std::optional<std::string> ParseSendFilecoinTransaction(
    const base::Value& json_value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
