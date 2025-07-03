/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_

#include <string>
#include <vector>

#include "base/values.h"

namespace brave_wallet::eth {

// Request chainId for a network.
std::string eth_chainId();
// Returns the current price per gas in wei.
std::string eth_gasPrice();
// Returns the number of most recent block.
std::string eth_blockNumber();
// Returns the fee history.
std::string eth_feeHistory(const std::string& num_blocks,
                           const std::string& head,
                           const std::vector<double>& reward_percentiles);
// Returns the balance of the account of given address.
std::string eth_getBalance(const std::string& address,
                           const std::string& quantity_tag);
// Returns the number of transactions sent from an address.
std::string eth_getTransactionCount(const std::string& address,
                                    const std::string& quantity_tag);
// Returns code at a given address.
std::string eth_getCode(const std::string& address,
                        const std::string& quantity_tag);
// Creates new message call transaction or a contract creation for signed
// transactions.
std::string eth_sendRawTransaction(const std::string& raw_transaction);
// Executes a new message call immediately without creating a transaction on the
// block chain.
std::string eth_call(const std::string& from_address,
                     const std::string& to_address,
                     const std::string& gas,
                     const std::string& gas_price,
                     const std::string& value,
                     const std::string& data,
                     const std::string& quantity_tag);
std::string eth_call(const std::string& to_address, const std::string& data);
// Generates and returns an estimate of how much gas is necessary to allow the
// transaction to complete. The transaction will not be added to the blockchain.
// Note that the estimate may be significantly more than the amount of gas
// actually used by the transaction, for a variety of reasons including EVM
// mechanics and node performance.
//
// Some EVM clients allow passing an optional block parameter called
// QUANTITY|TAG, however the official specs in github.com/ethereum/eth1.0-specs
// do not. Therefore to support chains that follow the official specs, we do not
// allow specifying this parameter.
std::string eth_estimateGas(const std::string& from_address,
                            const std::string& to_address,
                            const std::string& gas,
                            const std::string& gas_price,
                            const std::string& value,
                            const std::string& data);
// Returns information about a block by block number.
std::string eth_getBlockByNumber(const std::string& quantity_tag,
                                 bool full_transaction_object);
// Returns the receipt of a transaction by transaction hash.
std::string eth_getTransactionReceipt(const std::string& transaction_hash);
// Returns an array of all logs matching a given filter object.
std::string eth_getLogs(base::Value::Dict filter_options);

}  // namespace brave_wallet::eth

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
