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
#include "net/base/data_url.h"

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

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  const auto* base_fee_list = result->FindList("baseFeePerGas");
  if (!base_fee_list)
    return false;
  for (const base::Value& entry : *base_fee_list) {
    const std::string* v = entry.GetIfString();
    // If we have unexpected output, so just return false
    if (!v)
      return false;
    base_fee_per_gas->push_back(*v);
  }

  base_fee_list = result->FindList("gasUsedRatio");
  if (!base_fee_list)
    return false;

  for (const base::Value& entry : *base_fee_list) {
    absl::optional<double> v = entry.GetIfDouble();
    // If we have unexpected output, so just return false
    if (!v)
      return false;
    gas_used_ratio->push_back(*v);
  }

  const auto* oldest_block_str = result->FindString("oldestBlock");
  if (!oldest_block_str)
    return false;
  *oldest_block = *oldest_block_str;

  const base::Value::List* reward_list_list = result->FindList("reward");
  if (reward_list_list) {
    for (const base::Value& item : *reward_list_list) {
      // If we have unexpected output, so just return false
      const auto* reward_list = item.GetIfList();
      if (!reward_list)
        return false;

      reward->push_back(std::vector<std::string>());
      std::vector<std::string>& current_reward_vector = reward->back();
      for (const auto& entry : *reward_list) {
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

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  if (const auto* transaction_hash = result->FindString("transactionHash"))
    receipt->transaction_hash = *transaction_hash;
  else
    return false;

  if (const auto* transaction_index = result->FindString("transactionIndex")) {
    if (!HexValueToUint256(*transaction_index, &receipt->transaction_index))
      return false;
  } else {
    return false;
  }

  if (const auto* block_number = result->FindString("blockNumber")) {
    if (!HexValueToUint256(*block_number, &receipt->block_number))
      return false;
  } else {
    return false;
  }

  if (const auto* block_hash = result->FindString("blockHash"))
    receipt->block_hash = *block_hash;
  else
    return false;

  std::string cumulative_gas_used;
  if (const auto* cumulative_gas_used =
          result->FindString("cumulativeGasUsed")) {
    if (!HexValueToUint256(*cumulative_gas_used, &receipt->cumulative_gas_used))
      return false;
  } else {
    return false;
  }

  if (const auto* gas_used = result->FindString("gasUsed")) {
    if (!HexValueToUint256(*gas_used, &receipt->gas_used))
      return false;
  } else {
    return false;
  }

  // contractAddress can be null
  if (const auto* contract_address = result->FindString("contractAddress")) {
    receipt->contract_address = *contract_address;
  }

  // TODO(darkdh): logs
#if 0
  const base::Value::List* logs = result->FindList("logs");
  if (!logs)
    return false;
  for (const std::string& entry : *logs)
    receipt->logs.push_back(entry);
#endif

  if (const auto* logs_bloom = result->FindString("logsBloom"))
    receipt->logs_bloom = *logs_bloom;
  else
    return false;

  if (const auto* status = result->FindString("status")) {
    uint32_t status_int = 0;
    if (!base::HexStringToUInt(*status, &status_int))
      return false;
    receipt->status = status_int == 1;
  } else {
    return false;
  }

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
  return ParseStringResult(json, content_hash);
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
