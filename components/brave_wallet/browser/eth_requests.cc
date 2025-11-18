/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_requests.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

namespace brave_wallet::eth {

std::string GetChainIdPayload() {
  return GetJsonRpcString("eth_chainId");
}

std::string GetGasPricePayload() {
  return GetJsonRpcString("eth_gasPrice");
}

std::string GetBlockNumberPayload() {
  return GetJsonRpcString("eth_blockNumber");
}

std::string GetFeeHistoryPayload(std::string_view num_blocks,
                                 std::string_view head,
                                 base::span<const double> reward_percentiles) {
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

std::string GetBalancePayload(std::string_view address,
                              std::string_view quantity_tag) {
  return GetJsonRpcString("eth_getBalance", address, quantity_tag);
}

std::string GetTransactionCountPayload(std::string_view address,
                                       std::string_view quantity_tag) {
  return GetJsonRpcString("eth_getTransactionCount", address, quantity_tag);
}

std::string GetCodePayload(std::string_view address,
                           std::string_view quantity_tag) {
  return GetJsonRpcString("eth_getCode", address, quantity_tag);
}

std::string GetSendRawTransactionPayload(std::string_view raw_transaction) {
  base::Value::List params;
  params.Append(base::Value(raw_transaction));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_sendRawTransaction", std::move(params));
  return GetJSON(dictionary);
}

std::string GetCallPayload(std::string_view to_address, std::string_view data) {
  base::Value::Dict transaction;
  transaction.Set("data", data);
  transaction.Set("to", to_address);

  base::Value::List params;
  params.Append(std::move(transaction));
  params.Append(kEthereumBlockTagLatest);
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_call", std::move(params));
  return GetJSON(dictionary);
}

std::string GetEstimateGasPayload(std::string_view from_address,
                                  std::string_view to_address,
                                  std::string_view gas,
                                  std::string_view gas_price,
                                  std::string_view val,
                                  std::string_view data) {
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

std::string eth_getBlockByHash(std::string_view block_hash,
                               bool full_transaction_object) {
  base::Value::List params;
  params.Append(base::Value(block_hash));
  params.Append(base::Value(full_transaction_object));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getBlockByHash", std::move(params));
  return GetJSON(dictionary);
}

std::string GetBlockByNumberPayload(std::string_view quantity_tag,
                                    bool full_transaction_object) {
  base::Value::List params;
  params.Append(base::Value(quantity_tag));
  params.Append(base::Value(full_transaction_object));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getBlockByNumber", std::move(params));
  return GetJSON(dictionary);
}

std::string GetTransactionReceiptPayload(std::string_view transaction_hash) {
  return GetJsonRpcString("eth_getTransactionReceipt", transaction_hash);
}

std::string GetLogsPayload(base::Value::Dict filter_options) {
  base::Value::List params;
  params.Append(std::move(filter_options));
  base::Value::Dict dictionary =
      GetJsonRpcDictionary("eth_getLogs", std::move(params));
  return GetJSON(dictionary);
}

}  // namespace brave_wallet::eth
