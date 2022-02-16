/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_requests.h"

#include <utility>

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace eth {

std::string web3_clientVersion() {
  return GetJsonRpcNoParams("web3_clientVersion");
}

std::string web3_sha3(const std::string& message) {
  return GetJsonRpc1Param("web3_sha3", ToHex(message));
}

std::string net_version() {
  return GetJsonRpcNoParams("net_version");
}

std::string net_listening() {
  return GetJsonRpcNoParams("net_listening");
}

std::string net_peerCount() {
  return GetJsonRpcNoParams("net_peerCount");
}

std::string eth_chainId() {
  return GetJsonRpcNoParams("eth_chainId");
}

std::string eth_protocolVersion() {
  return GetJsonRpcNoParams("eth_protocolVersion");
}

std::string eth_syncing() {
  return GetJsonRpcNoParams("eth_syncing");
}

std::string eth_coinbase() {
  return GetJsonRpcNoParams("eth_coinbase");
}

std::string eth_mining() {
  return GetJsonRpcNoParams("eth_mining");
}

std::string eth_hashrate() {
  return GetJsonRpcNoParams("eth_hashrate");
}

std::string eth_gasPrice() {
  return GetJsonRpcNoParams("eth_gasPrice");
}

std::string eth_accounts() {
  return GetJsonRpcNoParams("eth_accounts");
}

std::string eth_blockNumber() {
  return GetJsonRpcNoParams("eth_blockNumber");
}

std::string eth_feeHistory(int num_blocks,
                           const std::string& head,
                           const std::vector<double>& reward_percentiles) {
  base::Value percentile_values(base::Value::Type::LIST);
  for (size_t i = 0; i < reward_percentiles.size(); ++i) {
    percentile_values.Append(base::Value(reward_percentiles[i]));
  }

  base::Value params(base::Value::Type::LIST);
  params.Append(base::Value(num_blocks));
  params.Append(base::Value(head));
  params.Append(std::move(percentile_values));
  base::Value dictionary = GetJsonRpcDictionary("eth_feeHistory", &params);
  return GetJSON(dictionary);
}

std::string eth_getBalance(const std::string& address,
                           const std::string& quantity_tag) {
  return GetJsonRpc2Params("eth_getBalance", address, quantity_tag);
}

std::string eth_getStorageAt(const std::string& address,
                             const std::string& quantity,
                             const std::string& quantity_tag) {
  return GetJsonRpc3Params("eth_getStorageAt", address, quantity, quantity_tag);
}

std::string eth_getTransactionCount(const std::string& address,
                                    const std::string& quantity_tag) {
  return GetJsonRpc2Params("eth_getTransactionCount", address, quantity_tag);
}

std::string eth_getBlockTransactionCountByHash(const std::string& block_hash) {
  return GetJsonRpc1Param("eth_getBlockTransactionCountByHash", block_hash);
}

std::string eth_getBlockTransactionCountByNumber(
    const std::string& quantity_tag) {
  return GetJsonRpc1Param("eth_getBlockTransactionCountByNumber", quantity_tag);
}

std::string eth_getUncleCountByBlockHash(const std::string& block_hash) {
  return GetJsonRpc1Param("eth_getUncleCountByBlockHash", block_hash);
}

std::string eth_getUncleCountByBlockNumber(const std::string& quantity_tag) {
  return GetJsonRpc1Param("eth_getUncleCountByBlockNumber", quantity_tag);
}

std::string eth_getCode(const std::string& address,
                        const std::string& quantity_tag) {
  return GetJsonRpc2Params("eth_getCode", address, quantity_tag);
}

std::string eth_sign(const std::string& address,
                     const std::string& encoded_message) {
  return GetJsonRpc2Params("eth_sign", address, encoded_message);
}

std::string eth_signTransaction(const std::string& from_address,
                                const std::string& to_address,
                                const std::string& gas,
                                const std::string& gas_price,
                                const std::string& val,
                                const std::string& data,
                                const std::string& nonce) {
  base::Value params(base::Value::Type::LIST);
  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetKey("data", base::Value(data));
  transaction.SetKey("from", base::Value(from_address));
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  AddKeyIfNotEmpty(&transaction, "to", to_address);
  AddKeyIfNotEmpty(&transaction, "value", val);
  AddKeyIfNotEmpty(&transaction, "nonce", nonce);
  params.Append(std::move(transaction));
  base::Value dictionary = GetJsonRpcDictionary("eth_signTransaction", &params);
  return GetJSON(dictionary);
}

std::string eth_sendTransaction(const std::string& from_address,
                                const std::string& to_address,
                                const std::string& gas,
                                const std::string& gas_price,
                                const std::string& val,
                                const std::string& data,
                                const std::string& nonce) {
  base::Value params(base::Value::Type::LIST);
  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetKey("data", base::Value(data));
  transaction.SetKey("from", base::Value(from_address));
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  AddKeyIfNotEmpty(&transaction, "to", to_address);
  AddKeyIfNotEmpty(&transaction, "value", val);
  AddKeyIfNotEmpty(&transaction, "nonce", nonce);
  params.Append(std::move(transaction));
  base::Value dictionary = GetJsonRpcDictionary("eth_sendTransaction", &params);
  return GetJSON(dictionary);
}

std::string eth_sendRawTransaction(const std::string& raw_transaction) {
  base::Value params(base::Value::Type::LIST);
  params.Append(base::Value(raw_transaction));
  base::Value dictionary =
      GetJsonRpcDictionary("eth_sendRawTransaction", &params);
  return GetJSON(dictionary);
}

std::string eth_call(const std::string& from_address,
                     const std::string& to_address,
                     const std::string& gas,
                     const std::string& gas_price,
                     const std::string& val,
                     const std::string& data,
                     const std::string& quantity_tag) {
  base::Value params(base::Value::Type::LIST);
  base::Value transaction(base::Value::Type::DICTIONARY);
  AddKeyIfNotEmpty(&transaction, "data", data);
  AddKeyIfNotEmpty(&transaction, "from", from_address);
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  transaction.SetKey("to", base::Value(to_address));
  AddKeyIfNotEmpty(&transaction, "value", val);
  params.Append(std::move(transaction));
  params.Append(std::move(quantity_tag));
  base::Value dictionary = GetJsonRpcDictionary("eth_call", &params);
  return GetJSON(dictionary);
}

std::string eth_estimateGas(const std::string& from_address,
                            const std::string& to_address,
                            const std::string& gas,
                            const std::string& gas_price,
                            const std::string& val,
                            const std::string& data) {
  base::Value params(base::Value::Type::LIST);
  base::Value transaction(base::Value::Type::DICTIONARY);
  AddKeyIfNotEmpty(&transaction, "data", data);
  AddKeyIfNotEmpty(&transaction, "from", from_address);
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  transaction.SetKey("to", base::Value(to_address));
  AddKeyIfNotEmpty(&transaction, "value", val);
  params.Append(std::move(transaction));
  base::Value dictionary = GetJsonRpcDictionary("eth_estimateGas", &params);
  return GetJSON(dictionary);
}

std::string eth_getBlockByHash(const std::string& block_hash,
                               bool full_transaction_object) {
  base::Value params(base::Value::Type::LIST);
  params.Append(base::Value(block_hash));
  params.Append(base::Value(full_transaction_object));
  base::Value dictionary = GetJsonRpcDictionary("eth_getBlockByHash", &params);
  return GetJSON(dictionary);
}

std::string eth_getBlockByNumber(const std::string& quantity_tag,
                                 bool full_transaction_object) {
  base::Value params(base::Value::Type::LIST);
  params.Append(base::Value(quantity_tag));
  params.Append(base::Value(full_transaction_object));
  base::Value dictionary =
      GetJsonRpcDictionary("eth_getBlockByNumber", &params);
  return GetJSON(dictionary);
}

std::string eth_getTransactionByHash(const std::string& transaction_hash) {
  return GetJsonRpc1Param("eth_getTransactionByHash", transaction_hash);
}

std::string eth_getTransactionByBlockHashAndIndex(
    const std::string& transaction_hash,
    const std::string& transaction_index) {
  return GetJsonRpc2Params("eth_getTransactionByBlockHashAndIndex",
                           transaction_hash, transaction_index);
}

std::string eth_getTransactionByBlockNumberAndIndex(
    const std::string& quantity_tag,
    const std::string& transaction_index) {
  return GetJsonRpc2Params("eth_getTransactionByBlockNumberAndIndex",
                           quantity_tag, transaction_index);
}

std::string eth_getTransactionReceipt(const std::string& transaction_hash) {
  return GetJsonRpc1Param("eth_getTransactionReceipt", transaction_hash);
}

std::string eth_getUncleByBlockHashAndIndex(const std::string& transaction_hash,
                                            const std::string& uncle_index) {
  return GetJsonRpc2Params("eth_getUncleByBlockHashAndIndex", transaction_hash,
                           uncle_index);
}

std::string eth_getUncleByBlockNumberAndIndex(const std::string& quantity_tag,
                                              const std::string& uncle_index) {
  return GetJsonRpc2Params("eth_getUncleByBlockNumberAndIndex", quantity_tag,
                           uncle_index);
}

std::string eth_getCompilers() {
  return GetJsonRpcNoParams("eth_getCompilers");
}

std::string eth_compileSolidity(const std::string& source_code) {
  return GetJsonRpc1Param("eth_compileSolidity", source_code);
}

std::string eth_compileLLL(const std::string& source_code) {
  return GetJsonRpc1Param("eth_compileLLL", source_code);
}

std::string eth_compileSerpent(const std::string& source_code) {
  return GetJsonRpc1Param("eth_compileSerpent", source_code);
}

std::string eth_newFilter(const std::string& from_block_quantity_tag,
                          const std::string& to_block_quantity_tag,
                          const std::string& address,
                          base::Value* topics) {
  base::Value params(base::Value::Type::LIST);
  base::Value filter_options(base::Value::Type::DICTIONARY);
  AddKeyIfNotEmpty(&filter_options, "address", address);
  AddKeyIfNotEmpty(&filter_options, "fromBlock", from_block_quantity_tag);
  AddKeyIfNotEmpty(&filter_options, "toBlock", to_block_quantity_tag);
  if (!topics->GetList().empty()) {
    filter_options.SetKey("topics", std::move(*topics));
  }
  params.Append(std::move(filter_options));
  base::Value dictionary = GetJsonRpcDictionary("eth_newFilter", &params);
  return GetJSON(dictionary);
}

std::string eth_newBlockFilter() {
  return GetJsonRpcNoParams("eth_newBlockFilter");
}

std::string eth_newPendingTransactionFilter() {
  return GetJsonRpcNoParams("eth_newPendingTransactionFilter");
}

std::string eth_uninstallFilter(const std::string& filter_id) {
  return GetJsonRpc1Param("eth_uninstallFilter", filter_id);
}

std::string eth_getFilterChanges(const std::string& filter_id) {
  return GetJsonRpc1Param("eth_getFilterChanges", filter_id);
}

std::string eth_getFilterLogs(const std::string& filter_id) {
  return GetJsonRpc1Param("eth_getFilterLogs", filter_id);
}

std::string eth_getLogs(const std::string& from_block_quantity_tag,
                        const std::string& to_block_quantity_tag,
                        const std::string& address,
                        base::Value* topics,
                        const std::string& block_hash) {
  base::Value params(base::Value::Type::LIST);
  base::Value filter_options(base::Value::Type::DICTIONARY);
  AddKeyIfNotEmpty(&filter_options, "address", address);
  AddKeyIfNotEmpty(&filter_options, "fromBlock", from_block_quantity_tag);
  AddKeyIfNotEmpty(&filter_options, "toBlock", to_block_quantity_tag);
  if (!topics->GetList().empty()) {
    filter_options.SetKey("topics", std::move(*topics));
  }
  AddKeyIfNotEmpty(&filter_options, "blockhash", block_hash);
  params.Append(std::move(filter_options));
  base::Value dictionary = GetJsonRpcDictionary("eth_getLogs", &params);
  return GetJSON(dictionary);
}

}  // namespace eth

}  // namespace brave_wallet
