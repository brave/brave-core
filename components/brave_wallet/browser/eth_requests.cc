/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_requests.h"

#include <utility>

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet::eth {

std::string eth_chainId() {
  return GetJsonRpcString("eth_chainId");
}

std::string eth_gasPrice() {
  return GetJsonRpcString("eth_gasPrice");
}

std::string eth_blockNumber() {
  return GetJsonRpcString("eth_blockNumber");
}

std::string eth_feeHistory(const std::string& num_blocks,
                           const std::string& head,
                           const std::vector<double>& reward_percentiles) {
  base::Value::List percentile_values;
  for (double reward_percentile : reward_percentiles) {
    percentile_values.Append(base::Value(reward_percentile));
  }

  base::Value::List params;
  params.Append(base::Value(num_blocks));
  params.Append(base::Value(head));
  params.Append(std::move(percentile_values));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_feeHistory", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_getBalance(const std::string& address,
                           const std::string& quantity_tag) {
  return GetJsonRpcString("eth_getBalance", address, quantity_tag);
}

std::string eth_getTransactionCount(const std::string& address,
                                    const std::string& quantity_tag) {
  return GetJsonRpcString("eth_getTransactionCount", address, quantity_tag);
}

std::string eth_getCode(const std::string& address,
                        const std::string& quantity_tag) {
  return GetJsonRpcString("eth_getCode", address, quantity_tag);
}

std::string eth_sendRawTransaction(const std::string& raw_transaction) {
  base::Value::List params;
  params.Append(base::Value(raw_transaction));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_sendRawTransaction", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_call(const std::string& from_address,
                     const std::string& to_address,
                     const std::string& gas,
                     const std::string& gas_price,
                     const std::string& val,
                     const std::string& data,
                     const std::string& quantity_tag) {
  base::Value::List params;
  base::Value::Dict transaction;
  AddKeyIfNotEmpty(&transaction, "data", data);
  AddKeyIfNotEmpty(&transaction, "from", from_address);
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  transaction.Set("to", base::Value(to_address));
  AddKeyIfNotEmpty(&transaction, "value", val);
  params.Append(std::move(transaction));
  params.Append(std::move(quantity_tag));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_call", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_call(const std::string& to_address, const std::string& data) {
  return eth::eth_call("", to_address, "", "", "", data, "latest");
}

std::string eth_estimateGas(const std::string& from_address,
                            const std::string& to_address,
                            const std::string& gas,
                            const std::string& gas_price,
                            const std::string& val,
                            const std::string& data) {
  base::Value::List params;
  base::Value::Dict transaction;
  AddKeyIfNotEmpty(&transaction, "data", data);
  AddKeyIfNotEmpty(&transaction, "from", from_address);
  AddKeyIfNotEmpty(&transaction, "gas", gas);
  AddKeyIfNotEmpty(&transaction, "gasPrice", gas_price);
  transaction.Set("to", base::Value(to_address));
  AddKeyIfNotEmpty(&transaction, "value", val);
  params.Append(std::move(transaction));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_estimateGas", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_getBlockByHash(const std::string& block_hash,
                               bool full_transaction_object) {
  base::Value::List params;
  params.Append(base::Value(block_hash));
  params.Append(base::Value(full_transaction_object));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getBlockByHash", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_getBlockByNumber(const std::string& quantity_tag,
                                 bool full_transaction_object) {
  base::Value::List params;
  params.Append(base::Value(quantity_tag));
  params.Append(base::Value(full_transaction_object));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getBlockByNumber", std::move(params));
  return GetJSON(dictionary);
}

std::string eth_getTransactionReceipt(const std::string& transaction_hash) {
  return GetJsonRpcString("eth_getTransactionReceipt", transaction_hash);
}

std::string eth_getLogs(base::Value::Dict filter_options) {
  base::Value::List params;
  params.Append(std::move(filter_options));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getLogs", std::move(params));
  return GetJSON(dictionary);
}

}  // namespace brave_wallet::eth
