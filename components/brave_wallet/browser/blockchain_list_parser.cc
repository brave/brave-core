/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <map>
#include <optional>
#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/blockchain_list_schemas.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace brave_wallet {

namespace {

bool ParseResultFromDict(const base::Value::Dict* response_dict,
                         const std::string& key,
                         std::string* output_val) {
  auto* val = response_dict->FindString(key);
  if (!val) {
    return false;
  }
  *output_val = *val;
  return true;
}

std::optional<double> ParseNullableStringAsDouble(const base::Value& value) {
  if (value.is_none()) {
    return std::nullopt;
  }

  double result;
  if (value.is_string()) {
    if (!base::StringToDouble(value.GetString(), &result)) {
      return std::nullopt;
    }

    return result;
  }

  return std::nullopt;
}

std::optional<uint32_t> ParseNullableStringAsUint32(const base::Value& value) {
  if (value.is_none()) {
    return std::nullopt;
  }

  uint32_t result;
  if (value.is_string()) {
    if (!base::StringToUint(value.GetString(), &result)) {
      return std::nullopt;
    }

    return result;
  }

  return std::nullopt;
}

std::optional<base::Value> ParseJsonToDict(const std::string& json) {
  std::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return std::nullopt;
  }
  return records_v;
}

std::string EmptyIfNull(const std::string* str) {
  if (str) {
    return *str;
  }
  return "";
}

std::optional<mojom::OnRampProvider> ParseProvider(
    const std::string& provider_str) {
  if (provider_str == "ramp") {
    return mojom::OnRampProvider::kRamp;
  } else if (provider_str == "sardine") {
    return mojom::OnRampProvider::kSardine;
  } else if (provider_str == "transak") {
    return mojom::OnRampProvider::kTransak;
  } else if (provider_str == "stripe") {
    return mojom::OnRampProvider::kStripe;
  } else if (provider_str == "coinbase") {
    return mojom::OnRampProvider::kCoinbase;
  }

  return std::nullopt;
}

void AddDappListToMap(
    const std::string& key,
    const blockchain_lists::DappList& dapp_list_from_component,
    DappListMap* dapp_lists) {
  std::vector<mojom::DappPtr> dapp_list;
  for (const auto& dapp_from_component : dapp_list_from_component.results) {
    auto dapp = mojom::Dapp::New();
    dapp->range = dapp_list_from_component.range;

    uint32_t dapp_id;
    if (!base::StringToUint(dapp_from_component.dapp_id, &dapp_id)) {
      continue;
    }
    dapp->id = dapp_id;

    dapp->name = dapp_from_component.name;
    dapp->description = dapp_from_component.description;
    dapp->logo = dapp_from_component.logo;
    dapp->website = dapp_from_component.website;
    dapp->chains = std::vector<std::string>(dapp_from_component.chains.begin(),
                                            dapp_from_component.chains.end());
    dapp->categories =
        std::vector<std::string>(dapp_from_component.categories.begin(),
                                 dapp_from_component.categories.end());

    // If any of the metrics fields are null, skip the dapp
    std::optional<uint32_t> transactions =
        ParseNullableStringAsUint32(dapp_from_component.metrics.transactions);
    if (!transactions) {
      continue;
    }
    dapp->transactions = *transactions;

    std::optional<uint32_t> uaw =
        ParseNullableStringAsUint32(dapp_from_component.metrics.uaw);
    if (!uaw) {
      continue;
    }
    dapp->uaw = *uaw;

    std::optional<double> volume =
        ParseNullableStringAsDouble(dapp_from_component.metrics.volume);
    if (!volume) {
      continue;
    }
    dapp->volume = *volume;

    std::optional<double> balance =
        ParseNullableStringAsDouble(dapp_from_component.metrics.balance);
    if (!balance) {
      continue;
    }
    dapp->balance = *balance;

    dapp_list.push_back(std::move(dapp));
  }

  (*dapp_lists)[key] = std::move(dapp_list);
}

void AddTokenToMaps(const blockchain_lists::Token& token,
                    OnRampTokensListMap* on_ramp_map,
                    OffRampTokensListMap* off_ramp_map) {
  auto blockchain_token = mojom::BlockchainToken::New();
  blockchain_token->contract_address = token.contract_address;
  blockchain_token->name = token.name;
  blockchain_token->logo = token.logo;
  blockchain_token->is_erc20 = token.is_erc20;
  blockchain_token->is_erc721 = token.is_erc721;
  blockchain_token->is_erc1155 = token.is_erc1155;
  // Not used for on_ramp or off_ramp tokens.
  blockchain_token->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  blockchain_token->is_nft = token.is_nft;
  blockchain_token->symbol = token.symbol;
  blockchain_token->decimals = token.decimals;
  blockchain_token->visible = token.visible;
  blockchain_token->token_id = token.token_id;
  blockchain_token->coingecko_id = token.coingecko_id;
  blockchain_token->chain_id = token.chain_id;
  blockchain_token->coin = static_cast<mojom::CoinType>(token.coin);

  for (const auto& provider_str : token.on_ramp_providers) {
    auto provider_opt = ParseProvider(provider_str);
    if (provider_opt.has_value()) {
      (*on_ramp_map)[*provider_opt].push_back(blockchain_token->Clone());
    }
  }

  for (const auto& provider_str : token.off_ramp_providers) {
    mojom::OffRampProvider provider;
    if (provider_str == "ramp") {
      provider = mojom::OffRampProvider::kRamp;
    } else {
      continue;
    }
    (*off_ramp_map)[provider].push_back(blockchain_token->Clone());
  }
}

}  // namespace

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

  std::optional<base::Value> records_v =
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

    blockchain_token->is_erc20 =
        blockchain_token_value->FindBool("erc20").value_or(false);

    blockchain_token->is_erc721 =
        blockchain_token_value->FindBool("erc721").value_or(false);

    blockchain_token->is_nft = blockchain_token->is_erc721;

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

    std::optional<int> decimals_opt =
        blockchain_token_value->FindInt("decimals");
    if (decimals_opt) {
      blockchain_token->decimals = *decimals_opt;
    } else {
      continue;
    }

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
    blockchain_token->visible = true;

    if (IsSPLToken(blockchain_token)) {
      bool is_token2022 =
          blockchain_token_value->FindBool("token2022").value_or(false);
      blockchain_token->spl_token_program =
          is_token2022 ? mojom::SPLTokenProgram::kToken2022
                       : mojom::SPLTokenProgram::kToken;
    } else {
      blockchain_token->spl_token_program =
          mojom::SPLTokenProgram::kUnsupported;
    }

    (*token_list_map)[GetTokenListKey(coin, blockchain_token->chain_id)]
        .push_back(std::move(blockchain_token));
  }

  return true;
}

std::optional<RampTokenListMaps> ParseRampTokenListMaps(
    const std::string& json) {
  // {
  //   "tokens" : [
  //      {
  //        "chain_id": "0x1",
  //        "coin": 60,
  //        "coingecko_id": "",
  //        "contract_address": "",
  //        "decimals": 18,
  //        "is_erc1155": false,
  //        "is_erc20": false,
  //        "is_erc721": false,
  //        "is_nft": false,
  //        "logo": "",
  //        "name": "Ethereum",
  //        "symbol": "ETH",
  //        "token_id": "",
  //        "visible": true,
  //        "on_ramp_providers": ["ramp", "sardine", "transak", "stripe"],
  //        "off_ramp_providers": []
  //      },
  //      {
  //        "chain_id": "0x38",
  //        "coin": 60,
  //        "coingecko_id": "",
  //        "contract_address": "",
  //        "decimals": 18,
  //        "is_erc1155": false,
  //        "is_erc20": true,
  //        "is_erc721": false,
  //        "is_nft": false,
  //        "logo": "",
  //        "name": "BNB",
  //        "symbol": "BNB",
  //        "token_id": "",
  //        "visible": true,
  //        "on_ramp_providers": ["ramp"],
  //        "off_ramp_providers": []
  //      },
  //      {
  //        "chain_id": "0x1",
  //        "coin": 60,
  //        "coingecko_id": "",
  //        "contract_address": "0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9",
  //        "decimals": 18,
  //        "is_erc1155": false,
  //        "is_erc20": true,
  //        "is_erc721": false,
  //        "is_nft": false,
  //        "logo": "aave.png",
  //        "name": "AAVE",
  //        "symbol": "AAVE",
  //        "token_id": "",
  //        "visible": true,
  //        "on_ramp_providers": ["sardine"],
  //        "off_ramp_providers": []
  //      },
  //      {
  //        "chain_id": "0xa",
  //        "coin": 60,
  //        "coingecko_id": "",
  //        "contract_address": "",
  //        "decimals": 18,
  //        "is_erc1155": false,
  //        "is_erc20": false,
  //        "is_erc721": false,
  //        "is_nft": false,
  //        "logo": "",
  //        "name": "Ethereum",
  //        "symbol": "ETH",
  //        "token_id": "",
  //        "visible": true,
  //        "on_ramp_providers": ["transak"],
  //        "off_ramp_providers": []
  //      }
  //   ]
  // }

  std::optional<base::Value> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  OnRampTokensListMap on_ramp_supported_tokens_lists;
  OffRampTokensListMap off_ramp_supported_tokens_lists;

  const auto tokens_list =
      blockchain_lists::OnRampTokenLists::FromValue(records_v->GetDict());
  if (!tokens_list) {
    return std::nullopt;
  }

  for (const auto& token : (*tokens_list).tokens) {
    AddTokenToMaps(token, &on_ramp_supported_tokens_lists,
                   &off_ramp_supported_tokens_lists);
  }

  return RampTokenListMaps{std::move(on_ramp_supported_tokens_lists),
                           std::move(off_ramp_supported_tokens_lists)};
}

std::optional<std::vector<mojom::OnRampCurrency>> ParseOnRampCurrencyLists(
    const std::string& json) {
  std::optional<base::Value> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  const auto on_ramp_supported_currencies_from_component =
      blockchain_lists::OnRampCurrencyLists::FromValue(records_v->GetDict());

  if (!on_ramp_supported_currencies_from_component) {
    return std::nullopt;
  }

  std::vector<mojom::OnRampCurrency> on_ramp_supported_currencies;
  for (const auto& currency :
       on_ramp_supported_currencies_from_component->currencies) {
    mojom::OnRampCurrency on_ramp_currency;

    on_ramp_currency.currency_code = currency.currency_code;
    on_ramp_currency.currency_name = currency.currency_name;

    for (const auto& provider_str : currency.providers) {
      auto provider_opt = ParseProvider(provider_str);
      if (!provider_opt) {
        continue;
      }
      on_ramp_currency.providers.push_back(*provider_opt);
    }

    on_ramp_supported_currencies.push_back(on_ramp_currency);
  }

  return on_ramp_supported_currencies;
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
    if (!chain_item) {
      continue;
    }

    int chain_id = chain_item->FindInt("chainId").value_or(0);
    if (!chain_id) {
      continue;
    }
    network->chain_id = base::StringPrintf("0x%x", chain_id);

    network->chain_name = EmptyIfNull(chain_item->FindString("name"));
    if (network->chain_name.empty()) {
      continue;
    }

    if (auto* block_explorer_list = chain_item->FindList("explorers")) {
      for (auto& item : *block_explorer_list) {
        if (auto* explorer = item.GetIfDict()) {
          if (auto* url = explorer->FindString("url")) {
            if (IsHTTPSOrLocalhostURL(*url)) {
              network->block_explorer_urls.push_back(*url);
            }
          }
        }
      }
    }
    if (network->block_explorer_urls.empty()) {
      continue;
    }

    if (auto* rpc_list = chain_item->FindList("rpc")) {
      for (auto& item : *rpc_list) {
        if (auto* url = item.GetIfString()) {
          if (IsHTTPSOrLocalhostURL(*url)) {
            network->rpc_endpoints.emplace_back(*url);
          }
        }
      }
    }
    if (network->rpc_endpoints.empty()) {
      continue;
    }
    network->active_rpc_endpoint_index =
        GetFirstValidChainURLIndex(network->rpc_endpoints);

    network->symbol = EmptyIfNull(
        chain_item->FindStringByDottedPath("nativeCurrency.symbol"));
    if (network->symbol.empty()) {
      continue;
    }
    network->symbol_name =
        EmptyIfNull(chain_item->FindStringByDottedPath("nativeCurrency.name"));
    if (network->symbol_name.empty()) {
      continue;
    }
    network->decimals =
        chain_item->FindIntByDottedPath("nativeCurrency.decimals").value_or(0);
    if (network->decimals == 0) {
      continue;
    }
    network->coin = mojom::CoinType::ETH;
    network->supported_keyrings =
        GetSupportedKeyringsForNetwork(network->coin, network->chain_id);

    result->push_back(std::move(network));
  }

  return true;
}

std::optional<DappListMap> ParseDappLists(const std::string& json) {
  // {
  //   "solana": {
  //     "success": true,
  //     "chain": "solana",
  //     "category": null,
  //     "range": "30d",
  //     "top": "100",
  //     "results": [
  //       {
  //         "dappId": "20419",
  //         "name": "GameTrade Market",
  //         "description": "Discover, buy, sell and trade in-game NFTs",
  //         "logo":
  //         "https://dashboard-assets.dappradar.com/document/20419/gametrademarket-dapp-marketplaces-matic-logo_e3e698e60ebd9bfe8ed1421bb41b890d.png",
  //         "link":
  //         "https://dappradar.com/solana/marketplaces/gametrade-market-2",
  //         "website": "https://gametrade.market/",
  //         "chains": [
  //           "polygon",
  //           "solana",
  //           "binance-smart-chain"
  //         ],
  //         "categories": [
  //           "marketplaces"
  //         ],
  //         "metrics": {
  //           "transactions": "1513120",
  //           "uaw": "917737",
  //           "volume": "32352.38",
  //           "balance": "3.81"
  //         }
  //       }
  //     ]
  //   },
  //   "ethereum": {
  //     "success": true,
  //     "chain": "ethereum",
  //     "category": null,
  //     "range": "30d",
  //     "top": "100",
  //     "results": [
  //       {
  //         "dappId": "7000",
  //         "name": "Uniswap V3",
  //         "description": "A protocol for trading and automated liquidity.",
  //         "logo":
  //         "https://dashboard-assets.dappradar.com/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png",
  //         "link": "https://dappradar.com/ethereum/exchanges/uniswap-v3",
  //         "website": "https://app.uniswap.org/#/swap",
  //         "chains": [
  //           "ethereum",
  //           "polygon",
  //           "optimism",
  //           "celo",
  //           "arbitrum",
  //           "binance-smart-chain"
  //         ],
  //         "categories": [
  //           "exchanges"
  //         ],
  //         "metrics": {
  //           "transactions": "3596443",
  //           "uaw": "507730",
  //           "volume": "42672855706.52",
  //           "balance": "1887202135.14"
  //         }
  //       }
  //     ]
  //   },
  //   ...
  // }

  std::optional<base::Value> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  auto dapp_lists_from_component =
      blockchain_lists::DappLists::FromValue(records_v->GetDict());
  if (!dapp_lists_from_component) {
    return std::nullopt;
  }

  DappListMap dapp_lists;
  AddDappListToMap(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId),
      dapp_lists_from_component->ethereum, &dapp_lists);
  AddDappListToMap(GetTokenListKey(mojom::CoinType::SOL, mojom::kSolanaMainnet),
                   dapp_lists_from_component->solana, &dapp_lists);
  AddDappListToMap(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kPolygonMainnetChainId),
      dapp_lists_from_component->polygon, &dapp_lists);
  AddDappListToMap(GetTokenListKey(mojom::CoinType::ETH,
                                   mojom::kBnbSmartChainMainnetChainId),
                   dapp_lists_from_component->binance_smart_chain, &dapp_lists);
  AddDappListToMap(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kOptimismMainnetChainId),
      dapp_lists_from_component->optimism, &dapp_lists);
  AddDappListToMap(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kAvalancheMainnetChainId),
      dapp_lists_from_component->avalanche, &dapp_lists);
  AddDappListToMap(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kFantomMainnetChainId),
      dapp_lists_from_component->fantom, &dapp_lists);

  return dapp_lists;
}

std::optional<CoingeckoIdsMap> ParseCoingeckoIdsMap(const std::string& json) {
  // {
  //   "0x1": {
  //     "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78": "0chain",
  //     "0xe41d2489571d322189246dafa5ebde1f4699f498": "0x",
  //     "0x5a3e6a77ba2f983ec0d371ea3b475f8bc0811ad5":
  //     "0x0-ai-ai-smart-contract",
  //     "0xfcdb9e987f9159dab2f507007d5e3d10c510aa70":
  //     "0x1-tools-ai-multi-tool",
  //     "0x37268c4f56ebb13dfae9c16d57d17579312d0ee1":
  //     "0xauto-io-contract-auto-deployer"
  //   }
  // }

  std::optional<base::Value> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  const base::Value::Dict* chain_ids = records_v->GetIfDict();
  if (!chain_ids) {
    return std::nullopt;
  }

  std::map<std::pair<std::string, std::string>, std::string> coingecko_ids_map;
  for (const auto chain_id_record : *chain_ids) {
    const auto& chain_id = base::ToLowerASCII(chain_id_record.first);

    const auto* contract_addresses = chain_id_record.second.GetIfDict();
    if (!contract_addresses) {
      return std::nullopt;
    }

    for (const auto contract_address_record : *contract_addresses) {
      const auto& contract_address =
          base::ToLowerASCII(contract_address_record.first);
      const auto* coingecko_id = contract_address_record.second.GetIfString();
      if (!coingecko_id) {
        return std::nullopt;
      }

      coingecko_ids_map[{chain_id, contract_address}] = *coingecko_id;
    }
  }

  return CoingeckoIdsMap(coingecko_ids_map.begin(), coingecko_ids_map.end());
}

std::optional<std::vector<std::string>> ParseOfacAddressesList(
    const std::string& json) {
  // {
  //   "addresses": [
  //     "t1MMXtBrSp1XG38Lx9cePcNUCJj5vdWfUWL",
  //     "t1WSKwCDL1QYRRUrCCknEs5tDLhtGVYu9KM",
  //     ...
  //   ]
  // }
  std::optional<base::Value> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  auto ofac_list_from_component =
      blockchain_lists::OfacAddressesList::FromValue(records_v->GetDict());
  if (!ofac_list_from_component) {
    return std::nullopt;
  }

  std::vector<std::string> ofac_list;
  for (const auto& address : (*ofac_list_from_component).addresses) {
    ofac_list.push_back(base::ToLowerASCII(address));
  }

  return ofac_list;
}

}  // namespace brave_wallet
