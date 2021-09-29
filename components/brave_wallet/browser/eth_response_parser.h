/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_

#include <string>
#include "base/values.h"

#include "brave/components/brave_wallet/browser/brave_wallet_types.h"

namespace brave_wallet {

bool ParseEthGetBlockNumber(const std::string& json, uint256_t* block_num);
// Returns the balance of the account of given address.
bool ParseEthGetBalance(const std::string& json, std::string* hex_balance);
bool ParseEthGetTransactionCount(const std::string& json, uint256_t* count);
bool ParseEthGetTransactionReceipt(const std::string& json,
                                   TransactionReceipt* receipt);
bool ParseEthSendRawTransaction(const std::string& json, std::string* tx_hash);
bool ParseEthCall(const std::string& json, std::string* result);
bool ParseEthEstimateGas(const std::string& json, std::string* result);
bool ParseEthGasPrice(const std::string& json, std::string* result);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_
