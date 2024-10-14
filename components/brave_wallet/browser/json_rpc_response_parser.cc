/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"

#include <optional>
#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet {

std::optional<std::string> ParseSingleStringResult(
    const base::Value& json_value) {
  auto result_v = ParseResultValue(json_value);
  if (!result_v) {
    return std::nullopt;
  }

  const std::string* result_str = result_v->GetIfString();
  if (!result_str) {
    return std::nullopt;
  }

  return *result_str;
}

std::optional<std::vector<uint8_t>> ParseDecodedBytesResult(
    const base::Value& json_value) {
  auto result_v = ParseResultValue(json_value);
  if (!result_v) {
    return std::nullopt;
  }

  const std::string* result_str = result_v->GetIfString();
  if (!result_str) {
    return std::nullopt;
  }

  return PrefixedHexStringToBytes(*result_str);
}

std::optional<base::Value> ParseResultValue(const base::Value& json_value) {
  if (!json_value.is_dict()) {
    return std::nullopt;
  }

  auto response =
      json_rpc_responses::RPCResponse::FromValue(json_value.GetDict());
  if (!response || !response->result) {
    return std::nullopt;
  }
  return std::move(*response->result);
}

std::optional<base::Value::Dict> ParseResultDict(
    const base::Value& json_value) {
  auto result = ParseResultValue(json_value);
  if (!result || !result->is_dict()) {
    return std::nullopt;
  }

  return std::move(result->GetDict());
}

std::optional<base::Value::List> ParseResultList(
    const base::Value& json_value) {
  auto result = ParseResultValue(json_value);
  if (!result || !result->is_list()) {
    return std::nullopt;
  }

  return std::move(result->GetList());
}

std::optional<bool> ParseBoolResult(const base::Value& json_value) {
  auto result = ParseSingleStringResult(json_value);
  if (!result) {
    return std::nullopt;
  }

  if (*result ==
      "0x0000000000000000000000000000000000000000000000000000000000000001") {
    return true;
  } else if (*result ==
             "0x000000000000000000000000000000000000000000000000000000000000000"
             "0") {
    return false;
  }

  return std::nullopt;
}

std::optional<std::string> ConvertUint64ToString(const std::string& path,
                                                 const std::string& json) {
  if (path.empty() || json.empty()) {
    return std::nullopt;
  }

  std::string converted_json(
      json::convert_uint64_value_to_string(path, json, true));
  if (converted_json.empty()) {
    return std::nullopt;
  }

  return converted_json;
}

std::optional<std::string> ConvertMultiUint64ToString(
    const std::vector<std::string>& paths,
    const std::string& json) {
  if (paths.empty() || json.empty()) {
    return std::nullopt;
  }

  std::string converted_json(json);
  for (const auto& path : paths) {
    auto result = ConvertUint64ToString(path, converted_json);
    if (!result) {
      return std::nullopt;
    }
    converted_json = std::move(*result);
  }

  return converted_json;
}

std::optional<std::string> ConvertMultiUint64InObjectArrayToString(
    const std::string& path_to_list,
    const std::string& path_to_object,
    const std::vector<std::string>& keys,
    const std::string& json) {
  if (path_to_list.empty() || json.empty() || keys.empty()) {
    return std::nullopt;
  }

  std::string converted_json(json);
  for (const auto& key : keys) {
    if (key.empty()) {
      return std::nullopt;
    }
    converted_json = json::convert_uint64_in_object_array_to_string(
        path_to_list, path_to_object, key, converted_json);
    if (converted_json.empty()) {
      return std::nullopt;
    }
  }

  return converted_json;
}

std::optional<std::string> ConvertInt64ToString(const std::string& path,
                                                const std::string& json) {
  if (path.empty() || json.empty()) {
    return std::nullopt;
  }

  std::string converted_json(
      json::convert_int64_value_to_string(path, json, true));
  if (converted_json.empty()) {
    return std::nullopt;
  }
  return converted_json;
}

bool GetUint64FromDictValue(const base::Value::Dict& dict_value,
                            const std::string& key,
                            bool nullable,
                            uint64_t* ret) {
  if (!ret) {
    return false;
  }

  const base::Value* value = dict_value.Find(key);
  if (!value) {
    return false;
  }

  if (nullable && value->is_none()) {
    *ret = 0;
    return true;
  }

  auto* string_value = value->GetIfString();
  if (!string_value || string_value->empty()) {
    return false;
  }

  return base::StringToUint64(*string_value, ret);
}

std::optional<std::string> ConvertAllNumbersToString(const std::string& path,
                                                     const std::string& json) {
  auto converted_json = json::convert_all_numbers_to_string(json, path);
  if (converted_json.empty()) {
    return std::nullopt;
  }

  return converted_json;
}

namespace ankr {

namespace {

std::optional<std::string> GetChainIdFromAnkrBlockchain(
    const std::string& blockchain) {
  auto& blockchains = GetAnkrBlockchains();
  for (const auto& entry : blockchains) {
    if (entry.second == blockchain) {
      return entry.first;
    }
  }

  return std::nullopt;
}

}  // namespace

std::optional<std::vector<mojom::AnkrAssetBalancePtr>>
ParseGetAccountBalanceResponse(const base::Value& json_value) {
  // {
  //   "jsonrpc": "2.0",
  //   "id": 1,
  //   "result": {
  //     "totalBalanceUsd": "4915134435857.581297310767673907",
  //     "assets": [
  //       {
  //         "blockchain": "polygon",
  //         "tokenName": "Matic",
  //         "tokenSymbol": "MATIC",
  //         "tokenDecimals": 18,
  //         "tokenType": "NATIVE",
  //         "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
  //         "balance": "120.275036899888325666",
  //         "balanceRawInteger": "120275036899888325666",
  //         "balanceUsd": "66.534394147826631446",
  //         "tokenPrice": "0.553185397924316979",
  //         "thumbnail": "https://polygon.svg"
  //       },
  //       {
  //         "blockchain": "polygon",
  //         "tokenName": "USD Coin (PoS)",
  //         "tokenSymbol": "USDC",
  //         "tokenDecimals": 6,
  //         "tokenType": "ERC20",
  //         "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
  //         "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
  //         "balance": "8.202765",
  //         "balanceRawInteger": "8202765",
  //         "balanceUsd": "8.202765",
  //         "tokenPrice": "1",
  //         "thumbnail": "https://usdc.png"
  //       }
  //     ]
  //   }
  // }

  auto result = ParseResultDict(json_value);
  if (!result) {
    return std::nullopt;
  }

  auto response =
      json_rpc_responses::AnkrGetAccountBalancesResult::FromValue(*result);
  if (!response) {
    return std::nullopt;
  }

  std::vector<mojom::AnkrAssetBalancePtr> balances;
  for (const auto& asset_value : response->assets) {
    auto chain_id = GetChainIdFromAnkrBlockchain(asset_value.blockchain);
    if (!chain_id) {
      continue;
    }

    auto ankr_asset_balance = mojom::AnkrAssetBalance::New();
    ankr_asset_balance->balance = asset_value.balance_raw_integer;
    ankr_asset_balance->formatted_balance = asset_value.balance;
    ankr_asset_balance->balance_usd = asset_value.balance_usd;
    ankr_asset_balance->price_usd = asset_value.token_price;

    auto asset = mojom::BlockchainToken::New();

    if (asset_value.token_type == "NATIVE") {
      asset->contract_address = "";
    } else if (asset_value.contract_address.has_value()) {
      asset->contract_address = asset_value.contract_address.value();
    } else {
      continue;
    }

    asset->name = asset_value.token_name;
    asset->logo = asset_value.thumbnail;
    asset->is_erc20 = asset_value.token_type == "ERC20";
    asset->is_erc721 = asset_value.token_type == "ERC721";
    asset->is_erc1155 = asset_value.token_type == "ERC1155";
    asset->is_nft = false;   // Reserved for Solana
    asset->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
    asset->is_spam = false;  // Reserved for NFTs
    asset->visible = true;
    asset->symbol = asset_value.token_symbol;
    if (!base::StringToInt(asset_value.token_decimals, &asset->decimals) ||
        asset->decimals < 0) {
      continue;
    }
    asset->chain_id = *chain_id;
    asset->coin = mojom::CoinType::ETH;

    ankr_asset_balance->asset = std::move(asset);

    balances.push_back(std::move(ankr_asset_balance));
  }

  return balances;
}
}  // namespace ankr

}  // namespace brave_wallet
