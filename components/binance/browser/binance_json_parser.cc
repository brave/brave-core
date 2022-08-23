/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/binance/browser/binance_json_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"

// static
// Response Format
// {
//   "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
//   "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
//   "scope": "read",
//   "token_type": "bearer",
//   "expires_in": 30714
// }
//
bool BinanceJSONParser::GetTokensFromJSON(
    const std::string& json, std::string *value, std::string type) {
  if (!value) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* token = records_v->GetDict().FindString(type);

  if (token) {
    *value = *token;
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": [
//        {
//            "asset": "ADA",
//            "free": "0.00000000",
//            "locked": "0.00000000",
//            "freeze": "1.00000000",
//            "withdrawing": "0.00000000",
//            "btcValuation": "0.00000000",
//            "fiatValuation": "0.00000000"
//        }
//    ]
// }
//
bool BinanceJSONParser::GetAccountBalancesFromJSON(
    const std::string& json, BinanceAccountBalances* balances) {
  if (!balances) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* pv_arr = records_v->GetDict().FindList("data");
  if (!pv_arr) {
    return false;
  }

  for (const base::Value& val : *pv_arr) {
    DCHECK(val.is_dict());
    const auto* asset = val.GetDict().FindString("asset");
    const auto* free_amount = val.GetDict().FindString("free");
    const auto* btc_val = val.GetDict().FindString("btcValuation");
    const auto* fiat_val = val.GetDict().FindString("fiatValuation");

    if (!asset || !free_amount || !btc_val || !fiat_val) {
      continue;
    }

    std::vector<std::string> balance_data;
    balance_data.push_back(*free_amount);
    balance_data.push_back(*btc_val);
    balance_data.push_back(*fiat_val);

    balances->insert({*asset, balance_data});
  }

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//      "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
//      "quotePrice": "1094.01086957",
//      "tradeFee": "8",
//      "railFee": "0",
//      "totalFee": "8",
//      "totalAmount": "100649",
//      "showPrice": "1094.01086957"
//    },
// }
bool BinanceJSONParser::GetQuoteInfoFromJSON(
    const std::string& json, std::string *quote_id,
    std::string *quote_price, std::string *total_fee,
    std::string *total_amount) {
  DCHECK(quote_id);
  DCHECK(quote_price);
  DCHECK(total_fee);
  DCHECK(total_amount);

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  const auto* data_dict = response_dict.FindDict("data");
  if (!data_dict) {
    return false;
  }

  // if (!data_dict->GetString("quoteId", quote_id) ||
  //     !data_dict->GetString("quotePrice", quote_price) ||
  //     !data_dict->GetString("totalFee", total_fee) ||
  //     !data_dict->GetString("totalAmount", total_amount)) {
  //   return false;
  // }

  const auto* local_quote_id = data_dict->FindString("quoteId");
  const auto* local_quote_price = data_dict->FindString("quotePrice");
  const auto* local_total_fee = data_dict->FindString("totalFee");
  const auto* local_total_amount = data_dict->FindString("totalAmount");
  if (!local_quote_id || !local_quote_price || !local_total_fee ||
      !local_total_amount)
    return false;

  *quote_id = *local_quote_id;
  *quote_price = *local_quote_price;
  *total_fee = *local_total_fee;
  *total_amount = *local_total_amount;

  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//      "coin": "BTC",
//      "address": "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
//      "tag": "",
//      "url": "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
//      "time": 1566366289000
//    },
//    "success": true
// }
bool BinanceJSONParser::GetDepositInfoFromJSON(
    const std::string& json, std::string *address, std::string *tag) {
  DCHECK(address);
  DCHECK(tag);

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  const auto* data_dict = response_dict.FindDict("data");
  if (!data_dict) {
    return false;
  }

  const auto* local_tag = data_dict->FindString("tag");
  const auto* local_address = data_dict->FindString("address");

  if (!local_tag || !local_address) {
    return false;
  }

  *tag = *local_tag;
  *address = *local_address;
  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": {
//        "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
//        "status": "FAIL",
//        "orderId": "ab0ab6cfd62240d79e10347fc5000bc4",
//        "fromAsset": "BNB",
//        "toAsset": "TRX",
//        "sourceAmount": 100,
//        "obtainAmount": 100649,
//        "tradeFee": 8,
//        "price": 1094.01086957,
//        "feeType": 1,
//        "feeRate": 0.08000000,
//        "fixFee": 13.00000000
//    },
//    "success": true
// }
bool BinanceJSONParser::GetConfirmStatusFromJSON(
    const std::string& json, std::string *error_message,
    bool* success_status) {
  if (!error_message || !success_status) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  const auto* data_dict = response_dict.FindDict("data");
  if (!data_dict) {
    const auto* message = response_dict.FindString("message");
    if (!message) {
      return false;
    }
    *success_status = false;
    *error_message = *message;
    return true;
  }

  *success_status = true;
  return true;
}

// static
// Response Format:
// {
//   "code":"000000",
//   "message":null,
//   "data":[{
//     "assetCode":"BTC",
//     "assetName":"Bitcoin",
//     "logoUrl":"https://bin.bnbstatic.com/images/20191211/fake.png",
//     "size":"6",
//     "order":0,
//     "freeAsset":"0.00508311",
//     "subSelector":[{
//       "assetCode":"BNB",
//       "assetName":"BNB",
//       "logoUrl":"https://bin.bnbstatic.com/images/fake.png",
//       "size":"2",
//       "order":1,
//       "perTimeMinLimit":"0.00200000",
//       "perTimeMaxLimit":"1.00000000",
//       "dailyMaxLimit":"10.00000000",
//       "hadDailyLimit":"0",
//       "needMarket":true,
//       "feeType":1,
//       "feeRate":"0.00050000",
//       "fixFee":"1.00000000",
//       "feeCoin":"BTC",
//       "forexRate":"1.00000000",
//       "expireTime":30
//     }]
//   }],
//   "success":true
// }
bool BinanceJSONParser::GetConvertAssetsFromJSON(const std::string& json,
    BinanceConvertAsserts* assets) {
  if (!assets) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* data_arr = records_v->GetDict().FindList("data");
  if (data_arr) {
    for (const base::Value& val : *data_arr) {
      DCHECK(val.is_dict());
      const auto* asset_symbol = val.GetDict().FindString("assetCode");
      if (asset_symbol) {
        std::vector<std::map<std::string, std::string>> sub_selectors;
        const auto* selectors = val.GetDict().FindList("subSelector");
        if (selectors) {
          for (const base::Value& selector : *selectors) {
            DCHECK(selector.is_dict());
            std::map<std::string, std::string> sub_selector;
            const auto* sub_code = selector.GetDict().FindString("assetCode");
            const auto* min_limit =
                selector.GetDict().FindString("perTimeMinLimit");
            if (sub_code && min_limit) {
              sub_selector.insert({"asset", *sub_code});
              sub_selector.insert({"minAmount", *min_limit});
            }
            sub_selectors.push_back(sub_selector);
          }
          assets->insert({*asset_symbol, sub_selectors});
        }
      }
    }
  }
  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "data": true,// true means clear access_token success
//    "success": true
// }
bool BinanceJSONParser::RevokeTokenFromJSON(
    const std::string& json,
    bool* success_status) {
  DCHECK(success_status);
  if (!success_status) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  absl::optional<bool> success_optional = response_dict.FindBool("success");
  if (!success_optional.has_value())
    return false;

  *success_status = success_optional.value();
  return true;
}

// static
// Response Format:
// {
//    "code": "000000",
//    "message": null,
//    "messageDetail": null,
//    "success": true,
//    "data": [
//        {
//            "coin": "CTR",
//            "networkList": [
//                {
//                    "coin": "CTR",
//                    "network": "ETH"
//                }
//            ]
//        }
//    ]
// }
//
bool BinanceJSONParser::GetCoinNetworksFromJSON(
    const std::string& json, BinanceCoinNetworks* networks) {
  if (!networks) {
    return false;
  }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* data_arr = records_v->GetDict().FindList("data");
  if (!data_arr) {
    return false;
  }

  for (const base::Value& coin : *data_arr) {
    DCHECK(coin.is_dict());
    const auto* coin_name = coin.GetDict().FindString("coin");
    if (!coin_name) {
      return false;
    }

    const auto* network_list = coin.GetDict().FindList("networkList");
    if (!network_list) {
      return false;
    }

    for (const base::Value& network : *network_list) {
      DCHECK(network.is_dict());
      const auto* network_name = network.GetDict().FindString("network");
      const auto is_default = network.GetDict().FindBool("isDefault");

      if (is_default.has_value() && is_default.value() && network_name) {
        networks->insert({*coin_name, *network_name});
        break;
      }
    }
  }

  return true;
}
