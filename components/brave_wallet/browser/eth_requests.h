/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_

#include <string>
#include <vector>

#include "base/values.h"

namespace brave_wallet {

namespace eth {

// Returns the current client version.
std::string web3_clientVersion();
// Returns Keccak-256 (not the standardized SHA3-256) of the given data.
std::string web3_sha3(const std::string& message);
// Returns the current network id.
std::string net_version();
// Returns true if client is actively listening for network connections.
std::string net_listening();
// Returns number of peers currently connected to the client.
std::string net_peerCount();
// Request chainId for a network.
std::string eth_chainId();
// Returns the current ethereum protocol version.
std::string eth_protocolVersion();
// Returns an object with data about the sync status or false.
std::string eth_syncing();
// Returns the client coinbase address.
// Not available on Infura
std::string eth_coinbase();
// Returns true if client is actively mining new blocks.
std::string eth_mining();
// Returns the number of hashes per second that the node is mining with.
std::string eth_hashrate();
// Returns the current price per gas in wei.
std::string eth_gasPrice();
// Returns a list of addresses owned by client.
std::string eth_accounts();
// Returns the number of most recent block.
std::string eth_blockNumber();
// Returns the fee history.
std::string eth_feeHistory(const std::string& num_blocks,
                           const std::string& head,
                           const std::vector<double>& reward_percentiles);
// Returns the balance of the account of given address.
std::string eth_getBalance(const std::string& address,
                           const std::string& quantity_tag);
// Returns the value from a storage position at a given address.
std::string eth_getStorageAt(const std::string& address,
                             const std::string& quantity,
                             const std::string& quantity_tag);
// Returns the number of transactions sent from an address.
std::string eth_getTransactionCount(const std::string& address,
                                    const std::string& quantity_tag);
// Returns the number of transactions in a block from a block matching the given
// block hash.
std::string eth_getBlockTransactionCountByHash(const std::string& block_hash);
// Returns the number of transactions in a block matching the given block
// number.
std::string eth_getBlockTransactionCountByNumber(
    const std::string& quantity_tag);
// Returns the number of uncles in a block from a block matching the given block
// hash.
std::string eth_getUncleCountByBlockHash(const std::string& block_hash);
// Returns the number of uncles in a block from a block matching the given block
// number.
std::string eth_getUncleCountByBlockNumber(const std::string& quantity_tag);
// Returns code at a given address.
std::string eth_getCode(const std::string& address,
                        const std::string& quantity_tag);
// The sign method calculates an Ethereum specific signature with:
// sign(keccak256("\x19Ethereum Signed Message:\n" + len(message) + message))).
// Not available on Infura
std::string eth_sign(const std::string& address,
                     const std::string& encoded_message);
// Signs a transaction that can be submitted to the network at a later time
// using with eth_sendRawTransaction.
// Not available on Infura
std::string eth_signTransaction(const std::string& from_address,
                                const std::string& to_address,
                                const std::string& gas,
                                const std::string& gas_price,
                                const std::string& value,
                                const std::string& data,
                                const std::string& nonce);
// Creates new message call transaction or a contract creation, if the data
// field contains code.
// Not available on Infura
std::string eth_sendTransaction(const std::string& from_address,
                                const std::string& to_address,
                                const std::string& gas,
                                const std::string& gas_price,
                                const std::string& value,
                                const std::string& data,
                                const std::string& nonce);
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
// Returns information about a block by hash.
std::string eth_getBlockByHash(const std::string& block_hash,
                               bool full_transaction_object);
// Returns information about a block by block number.
std::string eth_getBlockByNumber(const std::string& quantity_tag,
                                 bool full_transaction_object);
// Returns the information about a transaction requested by transaction hash.
std::string eth_getTransactionByHash(const std::string& transaction_hash);
// Returns information about a transaction by block hash and transaction index
// position.
std::string eth_getTransactionByBlockHashAndIndex(
    const std::string& transaction_hash,
    const std::string& transaction_index);
// Returns information about a transaction by block number and transaction index
// position.
std::string eth_getTransactionByBlockNumberAndIndex(
    const std::string& quantity_tag,
    const std::string& transaction_index);
// Returns the receipt of a transaction by transaction hash.
std::string eth_getTransactionReceipt(const std::string& transaction_hash);
// Returns information about a uncle of a block by hash and uncle index
// position.
std::string eth_getUncleByBlockHashAndIndex(
    const std::string& transaction_hash,
    const std::string& transaction_index);
// Returns information about a uncle of a block by number and uncle index
// position.
std::string eth_getUncleByBlockNumberAndIndex(
    const std::string& quantity_tag,
    const std::string& transaction_index);
// Returns a list of available compilers in the client.
// Not available on Infura
std::string eth_getCompilers();
// Returns compiled solidity code.
// Not available on Infura
std::string eth_compileSolidity(const std::string& source_code);
// Returns compiled LLL code.
// Not available on Infura
std::string eth_compileLLL(const std::string& source_code);
// Returns compiled serpent code.
// Not available on Infura
std::string eth_compileSerpent(const std::string& source_code);
// Creates a filter object, based on filter options, to notify when the state
// changes (logs).
std::string eth_newFilter(const std::string& from_block_quantity_tag,
                          const std::string& to_block_quantity_tag,
                          const std::string& address,
                          base::Value::List topics);
// Creates a filter in the node, to notify when a new block arrives.
std::string eth_newBlockFilter();
// Creates a filter in the node, to notify when new pending transactions arrive.
// Not available on Infura
std::string eth_newPendingTransactionFilter();
// Uninstalls a filter with given id. Should always be called when watch is no
// longer needed.
std::string eth_uninstallFilter(const std::string& filter_id);
// Polling method for a filter, which returns an array of logs which occurred
// since last poll.
std::string eth_getFilterChanges(const std::string& filter_id);
// Returns an array of all logs matching filter with given id.
std::string eth_getFilterLogs(const std::string& filter_id);
// Returns an array of all logs matching a given filter object.
std::string eth_getLogs(base::Value::Dict filter_options);
// Returns the hash of the current block, the seedHash, and the boundary
// condition to be met (“target”).
std::string eth_getWork();

}  // namespace eth

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_REQUESTS_H_
