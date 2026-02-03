/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "base/values.h"

namespace brave_wallet::eth {

// Request chainId for a network.
std::string GetChainIdPayload();
// Returns the current price per gas in wei.
std::string GetGasPricePayload();
// Returns the number of most recent block.
std::string GetBlockNumberPayload();
// Returns the fee history.
std::string GetFeeHistoryPayload(std::string_view num_blocks,
                                 std::string_view head,
                                 base::span<const double> reward_percentiles);
// Returns the balance of the account of given address.
std::string GetBalancePayload(std::string_view address,
                              std::string_view quantity_tag);
// Returns the number of transactions sent from an address.
std::string GetTransactionCountPayload(std::string_view address,
                                       std::string_view quantity_tag);
// Returns code at a given address.
std::string GetCodePayload(std::string_view address,
                           std::string_view quantity_tag);
// Creates new message call transaction or a contract creation for signed
// transactions.
std::string GetSendRawTransactionPayload(std::string_view raw_transaction);
// Executes a new message call immediately without creating a transaction on the
// block chain.
std::string GetCallPayload(std::string_view to_address, std::string_view data);
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
std::string GetEstimateGasPayload(std::string_view from_address,
                                  std::string_view to_address,
                                  std::string_view gas,
                                  std::string_view gas_price,
                                  std::string_view value,
                                  std::string_view data);
// Returns information about a block by block number.
std::string GetBlockByNumberPayload(std::string_view quantity_tag,
                                    bool full_transaction_object);
// Returns the receipt of a transaction by transaction hash.
std::string GetTransactionReceiptPayload(std::string_view transaction_hash);
// Returns an array of all logs matching a given filter object.
std::string GetLogsPayload(base::DictValue filter_options);

}  // namespace brave_wallet::eth

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
