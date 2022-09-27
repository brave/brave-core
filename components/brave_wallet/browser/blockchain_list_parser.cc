/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace {

bool ParseResultFromDict(const base::Value::Dict* response_dict,
                         const std::string& key,
                         std::string* output_val) {
  auto* val = response_dict->FindString(key);
  if (!val)
    return false;
  *output_val = *val;
  return true;
}

std::string EmptyIfNull(const std::string* str) {
  if (str)
    return *str;
  return "";
}

}  // namespace

namespace brave_wallet {

bool ParseTokenList(const std::string& json,
                    TokenListMap* token_list_map,
                    mojom::CoinType coin) {
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

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  for (const auto blockchain_token_value_pair : response_dict) {
    auto blockchain_token = mojom::BlockchainToken::New();
    blockchain_token->contract_address = blockchain_token_value_pair.first;
    const auto* blockchain_token_value =
        blockchain_token_value_pair.second.GetIfDict();
    if (!blockchain_token_value) {
      return false;
    }

    absl::optional<bool> is_erc20_opt =
        blockchain_token_value->FindBool("erc20");
    if (is_erc20_opt)
      blockchain_token->is_erc20 = *is_erc20_opt;
    else
      blockchain_token->is_erc20 = false;

    absl::optional<bool> is_erc721_opt =
        blockchain_token_value->FindBool("erc721");
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
        blockchain_token_value->FindInt("decimals");
    if (decimals_opt)
      blockchain_token->decimals = *decimals_opt;
    else
      continue;

    // chain_id is only optional for ETH mainnet token lists.
    blockchain_token->chain_id = "0x1";
    if (!ParseResultFromDict(blockchain_token_value, "chainId",
                             &blockchain_token->chain_id) &&
        coin != mojom::CoinType::ETH) {
      continue;
    }

    ParseResultFromDict(blockchain_token_value, "coingeckoId",
                        &blockchain_token->coingecko_id);

    blockchain_token->coin = coin;
    (*token_list_map)[GetTokenListKey(coin, blockchain_token->chain_id)]
        .push_back(std::move(blockchain_token));
  }

  return true;
}

std::string GetTokenListKey(mojom::CoinType coin, const std::string& chain_id) {
  return base::StrCat({GetPrefKeyForCoinType(coin), ".", chain_id});
}

bool ParseChainList(const std::string& json, ChainList* result) {
  DCHECK(result);

  // [
  //   {
  //     "name": "Ethereum Mainnet",
  //     "chain": "ETH",
  //     "icon": "ethereum",
  //     "rpc": [
  //       "https://mainnet.infura.io/v3/${INFURA_API_KEY}",
  //       "wss://mainnet.infura.io/ws/v3/${INFURA_API_KEY}",
  //       "https://api.mycryptoapi.com/eth",
  //       "https://cloudflare-eth.com"
  //     ],
  //     "faucets": [],
  //     "nativeCurrency": { "name": "Ether", "symbol": "ETH", "decimals": 18 },
  //     "infoURL": "https://ethereum.org",
  //     "shortName": "eth",
  //     "chainId": 1,
  //     "networkId": 1,
  //     "slip44": 60,
  //     "ens": { "registry": "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e" },
  //     "explorers": [
  //       {
  //         "name": "etherscan",
  //         "url": "https://etherscan.io",
  //         "standard": "EIP3091"
  //       }
  //     ]
  //   },
  // ]

  auto records_v = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    LOG(ERROR) << "Invalid response, could not parse JSON. "
               << records_v.error().message
               << ", line: " << records_v.error().line
               << ", col: " << records_v.error().column;
    return false;
  }

  const base::Value::List* chain_list = records_v->GetIfList();
  if (!chain_list) {
    return false;
  }

  for (const auto& list_item : *chain_list) {
    auto network = mojom::NetworkInfo::New();
    auto* chain_item = list_item.GetIfDict();
    if (!chain_item)
      continue;

    int chain_id = chain_item->FindInt("chainId").value_or(0);
    if (!chain_id)
      continue;
    network->chain_id = base::StringPrintf("0x%x", chain_id);

    network->chain_name = EmptyIfNull(chain_item->FindString("name"));
    if (network->chain_name.empty())
      continue;

    if (auto* block_explorer_list = chain_item->FindList("explorers")) {
      for (auto& item : *block_explorer_list) {
        if (auto* explorer = item.GetIfDict()) {
          if (auto* url = explorer->FindString("url")) {
            if (GURL(*url).is_valid()) {
              network->block_explorer_urls.push_back(*url);
            }
          }
        }
      }
    }
    if (network->block_explorer_urls.empty())
      continue;

    if (auto* rpc_list = chain_item->FindList("rpc")) {
      for (auto& item : *rpc_list) {
        if (auto* url = item.GetIfString()) {
          if (GURL(*url).is_valid()) {
            network->rpc_endpoints.emplace_back(*url);
          }
        }
      }
    }
    if (network->rpc_endpoints.empty())
      continue;
    network->active_rpc_endpoint_index =
        GetFirstValidChainURLIndex(network->rpc_endpoints);

    network->symbol = EmptyIfNull(
        chain_item->FindStringByDottedPath("nativeCurrency.symbol"));
    if (network->symbol.empty())
      continue;
    network->symbol_name =
        EmptyIfNull(chain_item->FindStringByDottedPath("nativeCurrency.name"));
    if (network->symbol_name.empty())
      continue;
    network->decimals =
        chain_item->FindIntByDottedPath("nativeCurrency.decimals").value_or(0);
    if (network->decimals == 0)
      continue;
    network->coin = mojom::CoinType::ETH;

    result->push_back(std::move(network));
  }

  return true;
}

}  // namespace brave_wallet
