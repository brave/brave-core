/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_response_parser.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "net/base/data_url.h"
#include "tools/json_schema_compiler/util.h"

namespace brave_wallet {

namespace eth {

bool ParseStringResult(const std::string& json, std::string* value) {
  DCHECK(value);

  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return false;

  size_t offset = 2 /* len of "0x" */ + 64 /* len of offset to array */;
  return brave_wallet::DecodeString(offset, result, value);
}

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

  auto value = ParseResultValue(json);
  if (!value)
    return false;

  auto fee_item_value =
      json_rpc_responses::EthGetFeeHistoryResult::FromValue(*value);
  if (!fee_item_value)
    return false;

  *base_fee_per_gas = fee_item_value->base_fee_per_gas;
  *gas_used_ratio = fee_item_value->gas_used_ratio;
  *oldest_block = fee_item_value->oldest_block;

  if (fee_item_value->reward) {
    for (const auto& reward_list_value : *fee_item_value->reward) {
      const auto* reward_list = reward_list_value.GetIfList();
      if (!reward_list)
        return false;
      reward->push_back(std::vector<std::string>());
      std::vector<std::string>& current_reward_vector = reward->back();
      if (!json_schema_compiler::util::PopulateArrayFromList(
              *reward_list, &current_reward_vector))
        return false;
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

  auto result = ParseResultValue(json);
  if (!result)
    return false;

  auto tx_receipt_value =
      json_rpc_responses::TransactionReceipt::FromValue(*result);
  if (!tx_receipt_value)
    return false;

  receipt->transaction_hash = tx_receipt_value->transaction_hash;
  if (!HexValueToUint256(tx_receipt_value->transaction_index,
                         &receipt->transaction_index))
    return false;

  if (!HexValueToUint256(tx_receipt_value->block_number,
                         &receipt->block_number))
    return false;

  receipt->block_hash = tx_receipt_value->block_hash;

  if (!HexValueToUint256(tx_receipt_value->cumulative_gas_used,
                         &receipt->cumulative_gas_used))
    return false;

  if (!HexValueToUint256(tx_receipt_value->gas_used, &receipt->gas_used))
    return false;

  if (tx_receipt_value->contract_address.is_string())
    receipt->contract_address = tx_receipt_value->contract_address.GetString();

  // TODO(darkdh): logs

  receipt->logs_bloom = tx_receipt_value->logs_bloom;

  uint32_t status_int = 0;
  if (!base::HexStringToUInt(tx_receipt_value->status, &status_int))
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

absl::optional<std::vector<std::string>> DecodeEthCallResponse(
    const std::string& data,
    const std::vector<std::string>& abi_types) {
  std::vector<uint8_t> response_bytes;
  if (!PrefixedHexStringToBytes(data, &response_bytes))
    return absl::nullopt;

  auto decoded = ABIDecode(abi_types, response_bytes);
  if (decoded == absl::nullopt)
    return absl::nullopt;

  const auto& args = std::get<1>(*decoded);
  if (args.size() != abi_types.size())
    return absl::nullopt;

  return args;
}

bool ParseEthEstimateGas(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

bool ParseEthGasPrice(const std::string& json, std::string* result) {
  return ParseSingleStringResult(json, result);
}

bool ParseEthGetLogs(const std::string& json, std::vector<Log>* logs) {
  DCHECK(logs);
  auto result = ParseResultList(json);
  if (!result)
    return false;

  DCHECK(result);

  for (const auto& logs_list_it : *result) {
    auto log_item_value =
        json_rpc_responses::EthGetLogsResult::FromValue(logs_list_it.Clone());
    if (!log_item_value)
      return false;

    Log log;
    log.address = log_item_value->address;
    log.block_hash = log_item_value->block_hash;

    uint256_t block_number_int = 0;
    if (!HexValueToUint256(log_item_value->block_number, &block_number_int))
      return false;
    log.block_number = block_number_int;
    log.data = log_item_value->data;

    uint32_t log_index_int = 0;
    if (!base::HexStringToUInt(log_item_value->log_index, &log_index_int))
      return false;
    log.log_index = log_index_int;
    log.removed = log_item_value->removed;
    log.transaction_hash = log_item_value->transaction_hash;

    uint32_t transaction_index_int = 0;
    if (!base::HexStringToUInt(log_item_value->transaction_index,
                               &transaction_index_int))
      return false;
    log.transaction_index = transaction_index_int;
    log.topics = log_item_value->topics;
    logs->push_back(log);
  }

  return true;
}

bool ParseEnsResolverContentHash(const std::string& json,
                                 std::vector<uint8_t>* content_hash) {
  content_hash->clear();

  std::string string_content_hash;
  if (!ParseStringResult(json, &string_content_hash)) {
    return false;
  }
  content_hash->assign(string_content_hash.begin(), string_content_hash.end());
  return true;
}

absl::optional<std::vector<std::string>>
ParseUnstoppableDomainsProxyReaderGetMany(const std::string& json) {
  std::string result;
  if (!ParseSingleStringResult(json, &result))
    return absl::nullopt;

  size_t offset = 2 /* len of "0x" */ + 64 /* len of offset to array */;
  if (offset > result.size())
    return absl::nullopt;

  std::vector<std::string> values;
  if (!brave_wallet::DecodeStringArray(result.substr(offset), &values))
    return absl::nullopt;

  return values;
}

absl::optional<std::string> ParseUnstoppableDomainsProxyReaderGet(
    const std::string& json) {
  std::string value;
  if (!ParseStringResult(json, &value)) {
    return absl::nullopt;
  }

  return value;
}

bool ParseTokenUri(const std::string& json, GURL* url) {
  std::string result;
  if (!ParseStringResult(json, &result)) {
    return false;
  }

  GURL result_url = GURL(result);
  if (!result_url.is_valid()) {
    return false;
  }

  *url = result_url;
  return true;
}

bool ParseDataURIAndExtractJSON(const GURL url, std::string* json) {
  std::string mime_type, charset, data;
  if (!net::DataURL::Parse(url, &mime_type, &charset, &data) || data.empty()) {
    return false;
  }

  if (mime_type != "application/json") {
    return false;
  }

  *json = data;
  return true;
}

}  // namespace eth

}  // namespace brave_wallet
