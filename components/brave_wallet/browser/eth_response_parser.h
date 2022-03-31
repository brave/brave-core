/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

namespace eth {

bool ParseAddressResult(const std::string& json, std::string* address);
bool ParseEthGetBlockNumber(const std::string& json, uint256_t* block_num);
bool ParseEthGetFeeHistory(const std::string& json,
                           std::vector<std::string>* base_fee_per_gas,
                           std::vector<double>* gas_used_ratio,
                           std::string* oldest_block,
                           std::vector<std::vector<std::string>>* reward);
// Returns the balance of the account of given address.
bool ParseEthGetBalance(const std::string& json, std::string* hex_balance);
bool ParseEthGetTransactionCount(const std::string& json, uint256_t* count);
bool ParseEthGetTransactionReceipt(const std::string& json,
                                   TransactionReceipt* receipt);
bool ParseEthSendRawTransaction(const std::string& json, std::string* tx_hash);
bool ParseEthCall(const std::string& json, std::string* result);
bool ParseEthEstimateGas(const std::string& json, std::string* result);
bool ParseEthGasPrice(const std::string& json, std::string* result);

bool ParseEnsResolverContentHash(const std::string& json,
                                 std::string* content_hash);
bool ParseUnstoppableDomainsProxyReaderGetMany(
    const std::string& json,
    std::vector<std::string>* values);

bool ParseUnstoppableDomainsProxyReaderGet(const std::string& json,
                                           std::string* value);

}  // namespace eth

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_RESPONSE_PARSER_H_
