/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

// TODO(apaymyshev): refactor utility methods to return std::optional instead
// of bool + out-parameter.

namespace brave_wallet {

// Returns the balance of the account of given address.
std::optional<std::string> ParseFilGetBalance(const base::Value& json_value);
// Returns the transaction count of given address.
std::optional<uint64_t> ParseFilGetTransactionCount(
    const base::Value& json_value);
// Returns Gas estimation values.
bool ParseFilEstimateGas(const base::Value& json_value,
                         std::string* gas_premium,
                         std::string* gas_fee_cap,
                         int64_t* gas_limit);
// Returns parsed chain head CID.
bool ParseFilGetChainHead(const base::Value& json_value, uint64_t* height);
// Returns parsed receipt exit code.
bool ParseFilStateSearchMsgLimited(const base::Value& json_value,
                                   const std::string& cid,
                                   int64_t* exit_code);
// Returns parsed transaction CID.
bool ParseSendFilecoinTransaction(const base::Value& json_value,
                                  std::string* tx_hash);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_RESPONSE_PARSER_H_
