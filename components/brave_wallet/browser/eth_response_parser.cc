/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_response_parser.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace {

bool ParseResult(const std::string& json, base::Value* result) {
  DCHECK(result);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::Value* result_v = response_dict->FindPath("result");
  if (!result_v)
    return false;

  *result = result_v->Clone();

  return true;
}

bool ParseSingleStringResult(const std::string& json, std::string* result) {
  DCHECK(result);

  base::Value result_v;
  if (!ParseResult(json, &result_v))
    return false;

  const std::string* result_str = result_v.GetIfString();
  if (!result_str)
    return false;

  *result = *result_str;

  return true;
}

}  // namespace

namespace brave_wallet {

bool ParseEthGetBlockNumber(const std::string& json, uint256_t* block_num) {
  std::string block_num_str;
  if (!ParseSingleStringResult(json, &block_num_str))
    return false;

  if (!HexValueToUint256(block_num_str, block_num))
    return false;

  return true;
}

bool ParseEthGetBalance(const std::string& json, std::string* hex_balance) {
  return ParseSingleStringResult(json, hex_balance);
}

bool ParseEthGetTransactionCount(const std::string& json, uint256_t* count) {
  std::string count_str;
  if (!ParseSingleStringResult(json, &count_str))
    return false;

  if (!HexValueToUint256(count_str, count))
    return false;

  return true;
}

bool ParseAddEthereumChainParameter(const std::string& json,
                                    AddEthereumChainParameter* result) {
  if (!result)
    return false;
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* request_dict;
  if (!records_v->GetAsDictionary(&request_dict)) {
    return false;
  }

  std::string method;
  if (!request_dict->GetString("method", &method))
    return false;

  DCHECK(method == "wallet_addEthereumChain");

  const base::ListValue* params = nullptr;
  if (!request_dict->GetList("params", &params))
    return false;
  const base::DictionaryValue* params_dict;
  params->GetList()[0].GetAsDictionary(&params_dict);
  if (!params_dict)
    return false;
  if (!params_dict->GetString("chainId", &result->chainId))
    return false;

  params_dict->GetString("chainName", &result->chainName);

  const base::ListValue* explorerUrlsList;
  if (params_dict->GetList("blockExplorerUrls", &explorerUrlsList)) {
    for (const auto& entry : explorerUrlsList->GetList())
      result->blockExplorerUrls.push_back(entry.GetString());
  }

  const base::ListValue* iconUrlsList;
  if (params_dict->GetList("iconUrls", &iconUrlsList)) {
    for (const auto& entry : iconUrlsList->GetList())
      result->iconUrls.push_back(entry.GetString());
  }

  const base::ListValue* rpcUrlsList;
  if (params_dict->GetList("rpcUrls", &rpcUrlsList)) {
    for (const auto& entry : rpcUrlsList->GetList())
      result->rpcUrls.push_back(entry.GetString());
  }

  const base::DictionaryValue* currency_dict;
  if (params_dict->GetDictionary("nativeCurrency", &currency_dict)) {
    currency_dict->GetString("name", &result->currency.name);
    currency_dict->GetString("symbol", &result->currency.symbol);
    result->currency.decimals =
        currency_dict->FindIntPath("decimals").value_or(0);
  }
  return true;
}

std::string ParseRequestMethodName(const std::string& json) {
  std::string method;
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return method;
  }

  const base::DictionaryValue* request_dict;
  if (!records_v->GetAsDictionary(&request_dict)) {
    return method;
  }

  if (!request_dict->GetString("method", &method))
    return method;
  return method;
}

bool ParseEthGetTransactionReceipt(const std::string& json,
                                   TransactionReceipt* receipt) {
  DCHECK(receipt);

  base::Value result;
  if (!ParseResult(json, &result))
    return false;
  const base::DictionaryValue* result_dict = nullptr;
  if (!result.GetAsDictionary(&result_dict))
    return false;
  DCHECK(result_dict);

  if (!result_dict->GetString("transactionHash", &receipt->transaction_hash))
    return false;
  std::string transaction_index;
  if (!result_dict->GetString("transactionIndex", &transaction_index))
    return false;
  if (!HexValueToUint256(transaction_index, &receipt->transaction_index))
    return false;

  std::string block_number;
  if (!result_dict->GetString("blockNumber", &block_number))
    return false;
  if (!HexValueToUint256(block_number, &receipt->block_number))
    return false;

  if (!result_dict->GetString("blockHash", &receipt->block_hash))
    return false;

  std::string cumulative_gas_used;
  if (!result_dict->GetString("cumulativeGasUsed", &cumulative_gas_used))
    return false;
  if (!HexValueToUint256(cumulative_gas_used, &receipt->cumulative_gas_used))
    return false;

  std::string gas_used;
  if (!result_dict->GetString("gasUsed", &gas_used))
    return false;
  if (!HexValueToUint256(gas_used, &receipt->gas_used))
    return false;

  // contractAddress can be null
  result_dict->GetString("contractAddress", &receipt->contract_address);

  // TODO(darkdh): logs
#if 0
  const base::ListValue* logs = nullptr;
  if (!result_dict->GetList("logs", &logs))
    return false;
  for (const std::string& entry : logs->GetList())
    receipt->logs.push_back(entry);
#endif

  if (!result_dict->GetString("logsBloom", &receipt->logs_bloom))
    return false;

  std::string status;
  if (!result_dict->GetString("status", &status))
    return false;
  uint32_t status_int = 0;
  if (!base::HexStringToUInt(status, &status_int))
    return false;
  receipt->status = status_int == 1;

  return true;
}

bool ParseEthSendRawTransaction(const std::string& json, std::string* tx_hash) {
  return ParseSingleStringResult(json, tx_hash);
}

bool ParseEthCall(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

}  // namespace brave_wallet
