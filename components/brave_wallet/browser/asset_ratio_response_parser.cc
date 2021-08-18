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
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

bool ParseAssetPrice(const std::string& json,
                     const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     std::vector<brave_wallet::mojom::AssetPricePtr>* values) {
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
    if (!from_asset_value->GetAsDictionary(&from_asset_dict)) {
      return false;
    }

    for (const std::string& to_asset : to_assets) {
      auto asset_price = brave_wallet::mojom::AssetPrice::New();
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

bool ParseAssetPriceHistory(
    const std::string& json,
    std::vector<brave_wallet::mojom::AssetTimePricePtr>* values) {
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

    double date_dbl;
    if (!date_value.GetAsDouble(&date_dbl)) {
      return false;
    }
    double price;
    if (!price_value.GetAsDouble(&price)) {
      return false;
    }

    base::Time date = base::Time::FromJsTime(date_dbl);
    auto asset_time_price = brave_wallet::mojom::AssetTimePrice::New();
    asset_time_price->date =
        base::TimeDelta::FromMilliseconds(date.ToJavaTime());
    asset_time_price->price = base::NumberToString(price);
    values->push_back(std::move(asset_time_price));
  }

  return true;
}

}  // namespace brave_wallet
