/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

bool ParseAssetPrice(const std::string& json,
                     const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     std::vector<mojom::AssetPricePtr>* values) {
  // Parses results like this:
  // /v2/relative/provider/coingecko/bat,chainlink/btc,usd/1w
  // {
  //  "payload": {
  //    "chainlink": {
  //      "btc": 0.00063075,
  //      "usd": 29.17,
  //      "btc_timeframe_change": -0.9999742658279261,
  //      "usd_timeframe_change": 0.1901162098990581
  //    },
  //    "bat": {
  //      "btc": 1.715e-05,
  //      "usd": 0.793188,
  //      "btc_timeframe_change": -0.9999993002916352,
  //      "usd_timeframe_change": -0.9676384677306338
  //    }
  //  },
  //  "lastUpdated": "2021-08-16T15:45:11.901Z"
  // }

  DCHECK(values);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  auto* payload = response_dict->FindPath("payload");
  if (!payload) {
    return false;
  }

  const base::DictionaryValue* payload_dict;
  if (!payload->GetAsDictionary(&payload_dict)) {
    return false;
  }

  for (const std::string& from_asset : from_assets) {
    const base::Value* from_asset_value =
        payload_dict->FindDictPath(from_asset);
    const base::DictionaryValue* from_asset_dict;
    if (!from_asset_value ||
        !from_asset_value->GetAsDictionary(&from_asset_dict)) {
      return false;
    }

    for (const std::string& to_asset : to_assets) {
      auto asset_price = mojom::AssetPrice::New();
      asset_price->from_asset = from_asset;
      asset_price->to_asset = to_asset;

      absl::optional<double> to_price =
          from_asset_dict->FindDoublePath(to_asset);
      if (!to_price) {
        return false;
      }
      asset_price->price = base::NumberToString(*to_price);
      std::string to_asset_timeframe_key =
          base::StringPrintf("%s_timeframe_change", to_asset.c_str());
      absl::optional<double> to_timeframe_change =
          from_asset_dict->FindDoublePath(to_asset_timeframe_key);
      if (!to_timeframe_change) {
        return false;
      }
      asset_price->asset_timeframe_change =
          base::NumberToString(*to_timeframe_change);

      values->push_back(std::move(asset_price));
    }
  }

  return true;
}

bool ParseAssetPriceHistory(const std::string& json,
                            std::vector<mojom::AssetTimePricePtr>* values) {
  DCHECK(values);

  // {  "payload":
  //   {
  //     "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
  //     "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
  //     "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
  //   }
  // }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  auto* payload = response_dict->FindPath("payload");
  if (!payload) {
    return false;
  }

  const base::DictionaryValue* payload_dict;
  if (!payload->GetAsDictionary(&payload_dict)) {
    return false;
  }

  auto* prices_list = payload->FindPath("prices");
  if (!prices_list) {
    return false;
  }

  const base::ListValue* list_of_lists;
  if (!prices_list->GetAsList(&list_of_lists)) {
    return false;
  }

  for (const auto& date_price_list_it : list_of_lists->GetList()) {
    const base::ListValue* date_price_list;
    if (!date_price_list_it.GetAsList(&date_price_list)) {
      return false;
    }
    auto it = date_price_list->GetList().begin();
    const auto& date_value = *it;
    const auto& price_value = *(++it);

    // Check whether date_value is convertible to a double first.
    if (!date_value.is_double() && !date_value.is_int())
      return false;
    double date_dbl = date_value.GetDouble();

    // Check whether price_value is convertible to a double first.
    if (!price_value.is_double() && !price_value.is_int())
      return false;
    double price = price_value.GetDouble();

    base::Time date = base::Time::FromJsTime(date_dbl);
    auto asset_time_price = mojom::AssetTimePrice::New();
    asset_time_price->date = base::Milliseconds(date.ToJavaTime());
    asset_time_price->price = base::NumberToString(price);
    values->push_back(std::move(asset_time_price));
  }

  return true;
}

std::string ParseEstimatedTime(const std::string& json) {
  // {
  //   "payload": {
  //     "status": "1",
  //     "message": "",
  //     "result": "3615"
  //   },
  //   "lastUpdated": "2021-09-22T21:45:40.015Z"
  // }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return "";
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return "";
  }

  const std::string* result = response_dict->FindStringPath("payload.result");
  return result ? *result : "";
}

mojom::GasEstimation1559Ptr ParseGasOracle(const std::string& json) {
  // {
  //   "payload": {
  //     "status": "1",
  //     "message": "",
  //     "result": {
  //       "LastBlock": "13243541",
  //       "SafeGasPrice": "47",
  //       "ProposeGasPrice": "48",
  //       "FastGasPrice": "48",
  //       "suggestBaseFee": "46.574033786",
  //       "gasUsedRatio": "0.27036175840958,0.0884828740801432,
  //           0.0426623303159149,0.972173412918789,0.319781207901446"
  //     }
  //   },
  //   "lastUpdated": "2021-09-22T21:45:40.015Z"
  // }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return nullptr;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return nullptr;
  }

  auto* result = response_dict->FindPath("payload.result");
  if (!result || !result->is_dict()) {
    return nullptr;
  }

  const std::string* safe_gas_price = result->FindStringKey("SafeGasPrice");
  if (!safe_gas_price || safe_gas_price->empty()) {
    return nullptr;
  }

  const std::string* proposed_gas_price =
      result->FindStringKey("ProposeGasPrice");
  if (!proposed_gas_price || proposed_gas_price->empty()) {
    return nullptr;
  }

  const std::string* fast_gas_price = result->FindStringKey("FastGasPrice");
  if (!fast_gas_price || fast_gas_price->empty()) {
    return nullptr;
  }

  const std::string* base_fee = result->FindStringKey("suggestBaseFee");
  if (!base_fee || base_fee->empty()) {
    return nullptr;
  }

  mojom::GasEstimation1559Ptr estimation = mojom::GasEstimation1559::New();

  // We save the original value in wei as the base_fee_per_gas, but use only
  // the integer part in gwei value to calculate the priority fee.
  size_t point = base_fee->find(".");
  std::string gwei_str = base_fee->substr(0, point);
  // Takes at most 9 digits and convert to wei, zeros will be padded if needed.
  std::string fractional =
      point == std::string::npos ? "" : base_fee->substr(point + 1, 9);
  size_t padding_len = 9 - fractional.size();
  std::string wei_str = gwei_str + fractional + std::string(padding_len, '0');

  uint64_t base_fee_gwei;
  if (!base::StringToUint64(gwei_str, &base_fee_gwei)) {
    return nullptr;
  }

  uint64_t base_fee_wei;
  if (!base::StringToUint64(wei_str, &base_fee_wei)) {
    return nullptr;
  }
  estimation->base_fee_per_gas = Uint256ValueToHex(base_fee_wei);

  uint64_t safe_gas_price_gwei;
  if (!base::StringToUint64(*safe_gas_price, &safe_gas_price_gwei)) {
    return nullptr;
  }
  estimation->slow_max_fee_per_gas =
      Uint256ValueToHex(static_cast<uint256_t>(safe_gas_price_gwei) *
                        static_cast<uint256_t>(1e9));
  estimation->slow_max_priority_fee_per_gas = Uint256ValueToHex(
      static_cast<uint256_t>(safe_gas_price_gwei - base_fee_gwei) *
      static_cast<uint256_t>(1e9));

  uint64_t proposed_gas_price_gwei;
  if (!base::StringToUint64(*proposed_gas_price, &proposed_gas_price_gwei)) {
    return nullptr;
  }
  estimation->avg_max_fee_per_gas =
      Uint256ValueToHex(static_cast<uint256_t>(proposed_gas_price_gwei) *
                        static_cast<uint256_t>(1e9));
  estimation->avg_max_priority_fee_per_gas = Uint256ValueToHex(
      static_cast<uint256_t>(proposed_gas_price_gwei - base_fee_gwei) *
      static_cast<uint256_t>(1e9));

  uint64_t fast_gas_price_gwei;
  if (!base::StringToUint64(*fast_gas_price, &fast_gas_price_gwei)) {
    return nullptr;
  }
  estimation->fast_max_fee_per_gas =
      Uint256ValueToHex(static_cast<uint256_t>(fast_gas_price_gwei) *
                        static_cast<uint256_t>(1e9));
  estimation->fast_max_priority_fee_per_gas = Uint256ValueToHex(
      static_cast<uint256_t>(fast_gas_price_gwei - base_fee_gwei) *
      static_cast<uint256_t>(1e9));

  return estimation;
}

mojom::BlockchainTokenPtr ParseTokenInfo(const std::string& json) {
  // {
  //   "payload": {
  //     "status": "1",
  //     "message": "OK",
  //     "result": [
  //       {
  //         "contractAddress": "0xdac17f958d2ee523a2206206994597c13d831ec7",
  //         "tokenName": "Tether USD",
  //         "symbol": "USDT",
  //         "divisor": "6",
  //         "tokenType": "ERC20",
  //         "totalSupply": "39828710009874796",
  //         "blueCheckmark": "true",
  //         "description": "Tether gives you the joint benefits of open...",
  //         "website": "https://tether.to/",
  //         "email": "support@tether.to",
  //         "blog": "https://tether.to/category/announcements/",
  //         "reddit": "",
  //         "slack": "",
  //         "facebook": "",
  //         "twitter": "https://twitter.com/Tether_to",
  //         "bitcointalk": "",
  //         "github": "",
  //         "telegram": "",
  //         "wechat": "",
  //         "linkedin": "",
  //         "discord": "",
  //         "whitepaper": "https://path/to/TetherWhitePaper.pdf",
  //         "tokenPriceUSD": "1.000000000000000000"
  //       }
  //     ]
  //   },
  //   "lastUpdated": "2021-12-09T22:02:23.187Z"
  // }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return nullptr;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict))
    return nullptr;

  const base::Value* result = response_dict->FindListPath("payload.result");
  if (!result || !result->is_list())
    return nullptr;

  const auto& result_list = result->GetList();
  if (result_list.size() != 1 || !result_list[0].is_dict())
    return nullptr;
  const base::Value* token = &result_list[0];

  const std::string* contract_address = token->FindStringKey("contractAddress");
  if (!contract_address)
    return nullptr;
  const auto eth_addr = EthAddress::FromHex(*contract_address);
  if (eth_addr.IsEmpty())
    return nullptr;

  const std::string* name = token->FindStringKey("tokenName");
  if (!name || name->empty())
    return nullptr;

  const std::string* symbol = token->FindStringKey("symbol");
  if (!symbol || symbol->empty())
    return nullptr;

  const std::string* decimals_string = token->FindStringKey("divisor");
  int decimals = 0;
  if (!decimals_string || !base::StringToInt(*decimals_string, &decimals))
    return nullptr;

  const std::string* token_type = token->FindStringKey("tokenType");
  if (!token_type)
    return nullptr;

  bool is_erc20 = base::EqualsCaseInsensitiveASCII(*token_type, "ERC20");
  bool is_erc721 = base::EqualsCaseInsensitiveASCII(*token_type, "ERC721");
  if (!is_erc20 && !is_erc721)  // unsupported token
    return nullptr;

  return mojom::BlockchainToken::New(eth_addr.ToChecksumAddress(), *name,
                                     "" /* logo */, is_erc20, is_erc721,
                                     *symbol, decimals, true, "", "");
}

}  // namespace brave_wallet
