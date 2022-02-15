/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_response_parser.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace eth {

bool ParseAddressResult(const std::string& json, std::string* address) {
  DCHECK(address);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  // Expected result: 0x prefix + 24 leading 0s + 40 characters for address.
  if (result.size() != 66) {
    return false;
  }

  size_t offset = 2 /* len of "0x" */ + 24 /* len of leading zeros */;
  *address = "0x" + result.substr(offset);

  auto eth_addr = EthAddress::FromHex("0x" + result.substr(offset));
  if (eth_addr.IsEmpty())
    return false;

  *address = eth_addr.ToChecksumAddress();
  return true;
}

bool ParseEthGetBlockNumber(const std::string& json, uint256_t* block_num) {
  std::string block_num_str;
  if (!brave_wallet::ParseSingleStringResult(json, &block_num_str))
    return false;

  if (!HexValueToUint256(block_num_str, block_num))
    return false;

  return true;
}

bool ParseEthGetFeeHistory(const std::string& json,
                           std::vector<std::string>* base_fee_per_gas,
                           std::vector<double>* gas_used_ratio,
                           std::string* oldest_block,
                           std::vector<std::vector<std::string>>* reward) {
  CHECK(base_fee_per_gas);
  CHECK(gas_used_ratio);
  CHECK(oldest_block);
  CHECK(reward);

  base_fee_per_gas->clear();
  gas_used_ratio->clear();
  oldest_block->clear();
  reward->clear();

  base::Value result;
  if (!ParseResult(json, &result))
    return false;
  const base::DictionaryValue* result_dict = nullptr;
  if (!result.GetAsDictionary(&result_dict))
    return false;
  DCHECK(result_dict);

  const base::ListValue* base_fee_list = nullptr;
  if (!result_dict->GetList("baseFeePerGas", &base_fee_list))
    return false;
  for (const base::Value& entry : base_fee_list->GetListDeprecated()) {
    const std::string* v = entry.GetIfString();
    // If we have unexpected output, so just return false
    if (!v)
      return false;
    base_fee_per_gas->push_back(*v);
  }

  if (!result_dict->GetList("gasUsedRatio", &base_fee_list))
    return false;

  for (const base::Value& entry : base_fee_list->GetListDeprecated()) {
    absl::optional<double> v = entry.GetIfDouble();
    // If we have unexpected output, so just return false
    if (!v)
      return false;
    gas_used_ratio->push_back(*v);
  }

  if (!result_dict->GetString("oldestBlock", oldest_block))
    return false;

  const base::ListValue* reward_list_list = nullptr;
  if (result_dict->GetList("reward", &reward_list_list)) {
    for (const base::Value& reward_list :
         reward_list_list->GetListDeprecated()) {
      // If we have unexpected output, so just return false
      if (!reward_list.is_list())
        return false;

      reward->push_back(std::vector<std::string>());
      std::vector<std::string>& current_reward_vector = reward->back();
      for (const auto& entry : reward_list.GetListDeprecated()) {
        const std::string* v = entry.GetIfString();
        // If we have unexpected output, so just return false
        if (!v)
          return false;
        current_reward_vector.push_back(*v);
      }
    }
  }

  return true;
}

bool ParseEthGetBalance(const std::string& json, std::string* hex_balance) {
  return brave_wallet::ParseSingleStringResult(json, hex_balance);
}

bool ParseEthGetTransactionCount(const std::string& json, uint256_t* count) {
  std::string count_str;
  if (!brave_wallet::ParseSingleStringResult(json, &count_str))
    return false;

  if (!HexValueToUint256(count_str, count))
    return false;

  return true;
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

bool ParseEthEstimateGas(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

bool ParseEthGasPrice(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

bool ParseEnsResolverContentHash(const std::string& json,
                                 std::string* content_hash) {
  DCHECK(content_hash);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  size_t offset = 2 /* len of "0x" */ + 64 /* len of offset to array */;
  return brave_wallet::DecodeString(offset, result, content_hash);
}

bool ParseUnstoppableDomainsProxyReaderGetMany(
    const std::string& json,
    std::vector<std::string>* values) {
  DCHECK(values);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  size_t offset = 2 /* len of "0x" */ + 64 /* len of offset to array */;
  if (offset > result.size())
    return false;

  return brave_wallet::DecodeStringArray(result.substr(offset), values);
}

bool ParseUnstoppableDomainsProxyReaderGet(const std::string& json,
                                           std::string* value) {
  DCHECK(value);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  size_t offset = 2 /* len of "0x" */ + 64 /* len of offset to array */;
  return brave_wallet::DecodeString(offset, result, value);
}

}  // namespace eth

}  // namespace brave_wallet
