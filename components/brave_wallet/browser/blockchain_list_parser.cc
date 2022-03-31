/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace {

bool ParseResultFromDict(const base::DictionaryValue* response_dict,
                         const std::string& key,
                         std::string* output_val) {
  auto* val = response_dict->FindStringKey(key);
  if (!val)
    return false;
  *output_val = *val;
  return true;
}

}  // namespace

namespace brave_wallet {

bool ParseTokenList(const std::string& json, TokenListMap* token_list_map) {
  DCHECK(token_list_map);

  // {
  //  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {
  //    "name": "Basic Attention Token",
  //    "logo": "bat.svg",
  //    "erc20": true,
  //    "symbol": "BAT",
  //    "decimals": 18
  //  },
  //  "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
  //    "name": "Crypto Kitties",
  //    "logo": "CryptoKitties-Kitty-13733.svg",
  //    "erc20": false,
  //    "erc721": true,
  //    "symbol": "CK",
  //    "decimals": 0
  //  },
  //  "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
  //    "name": "Uniswap",
  //    "logo": "uni.svg",
  //    "erc20": true,
  //    "symbol": "UNI",
  //    "decimals": 18,
  //    "chainId": "0x1"
  //  }
  // }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  auto& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  for (const auto blockchain_token_value_pair : response_dict->DictItems()) {
    auto blockchain_token = brave_wallet::mojom::BlockchainToken::New();
    blockchain_token->contract_address = blockchain_token_value_pair.first;
    const base::DictionaryValue* blockchain_token_value;
    if (!blockchain_token_value_pair.second.GetAsDictionary(
            &blockchain_token_value)) {
      return false;
    }

    absl::optional<bool> is_erc20_opt =
        blockchain_token_value->FindBoolKey("erc20");
    if (is_erc20_opt)
      blockchain_token->is_erc20 = *is_erc20_opt;
    else
      blockchain_token->is_erc20 = false;

    absl::optional<bool> is_erc721_opt =
        blockchain_token_value->FindBoolKey("erc721");
    if (is_erc721_opt)
      blockchain_token->is_erc721 = *is_erc721_opt;
    else
      blockchain_token->is_erc721 = false;

    if (!ParseResultFromDict(blockchain_token_value, "symbol",
                             &blockchain_token->symbol)) {
      continue;
    }
    if (!ParseResultFromDict(blockchain_token_value, "name",
                             &blockchain_token->name)) {
      return false;
    }
    ParseResultFromDict(blockchain_token_value, "logo",
                        &blockchain_token->logo);

    absl::optional<int> decimals_opt =
        blockchain_token_value->FindIntKey("decimals");
    if (decimals_opt)
      blockchain_token->decimals = *decimals_opt;
    else
      continue;

    std::string chain_id = "0x1";
    ParseResultFromDict(blockchain_token_value, "chainId", &chain_id);

    ParseResultFromDict(blockchain_token_value, "coingeckoId",
                        &blockchain_token->coingecko_id);

    (*token_list_map)[chain_id].push_back(std::move(blockchain_token));
  }

  return true;
}

}  // namespace brave_wallet
