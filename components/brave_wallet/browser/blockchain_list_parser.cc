/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <map>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/blockchain_list_schemas.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

namespace {

bool ParseResultFromDict(const base::DictValue* response_dict,
                         const std::string& key,
                         std::string* output_val) {
  auto* val = response_dict->FindString(key);
  if (!val) {
    return false;
  }
  *output_val = *val;
  return true;
}

bool ParseOptionalBoolFromDict(const base::DictValue* response_dict,
                               const std::string& key) {
  return response_dict->FindBool(key).value_or(false);
}

std::optional<base::DictValue> ParseJsonToDict(const std::string& json) {
  return base::JSONReader::ReadDict(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
}

std::string EmptyIfNull(const std::string* str) {
  if (str) {
    return *str;
  }
  return "";
}

}  // namespace

bool ParseTokenList(const std::string& json, TokenListMap* token_list_map) {
  DCHECK(token_list_map);

  // {
  //   "0x1": {
  //     "0xdac17f958d2ee523a2206206994597c13d831ec7": {
  //       "name": "Tether",
  //       "symbol": "usdt",
  //       "coingeckoId": "tether",
  //       "decimals": 6,
  //       "logo":
  //       "https://coin-images.coingecko.com/coins/images/325/large/Tether.png"
  //     },
  //     "0xb8c77482e45f1f44de1745f52c74426c631bdd52": {
  //       "name": "BNB",
  //       "symbol": "bnb",
  //       "coingeckoId": "binancecoin",
  //       "decimals": 18,
  //       "logo":
  //       "https://coin-images.coingecko.com/coins/images/825/large/bnb-icon2_2x.png"
  //     },
  //     "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
  //       "name": "Crypto Kitties",
  //       "logo": "CryptoKitties-Kitty-13733.svg",
  //       "erc20": false,
  //       "erc721": true,
  //       "symbol": "CK",
  //       "decimals": 0
  //     }
  //   }
  // }

  std::optional<base::DictValue> records_v = base::JSONReader::ReadDict(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    return false;
  }

  const auto& chain_dict = *records_v;
  // Iterate through chain IDs
  for (const auto chain_pair : chain_dict) {
    const std::string& chain_id = chain_pair.first;
    const auto* tokens_dict = chain_pair.second.GetIfDict();
    if (!tokens_dict) {
      continue;
    }

    // Determine coin type based on chain_id
    mojom::CoinType coin = mojom::CoinType::ETH;
    if (chain_id == mojom::kSolanaMainnet) {
      coin = mojom::CoinType::SOL;
    } else if (chain_id == mojom::kPolkadotMainnetAssetHub) {
      coin = mojom::CoinType::DOT;
    }

    // Iterate through tokens in this chain
    for (const auto token_pair : *tokens_dict) {
      auto blockchain_token = mojom::BlockchainToken::New();
      blockchain_token->contract_address = token_pair.first;
      const auto* token_info = token_pair.second.GetIfDict();
      if (!token_info) {
        continue;
      }

      // Parse required fields
      if (!ParseResultFromDict(token_info, "symbol",
                               &blockchain_token->symbol) ||
          !ParseResultFromDict(token_info, "name", &blockchain_token->name)) {
        continue;
      }

      // Parse optional fields
      ParseResultFromDict(token_info, "logo", &blockchain_token->logo);
      ParseResultFromDict(token_info, "coingeckoId",
                          &blockchain_token->coingecko_id);
      blockchain_token->is_erc721 =
          ParseOptionalBoolFromDict(token_info, "erc721");

      // Determining is_nft from blockchain list is not supported for Solana
      blockchain_token->is_nft = blockchain_token->is_erc721;
      bool is_token2022 = ParseOptionalBoolFromDict(token_info, "token2022");

      std::optional<int> decimals_opt = token_info->FindInt("decimals");
      if (!decimals_opt) {
        continue;
      }
      blockchain_token->decimals = *decimals_opt;

      // Set default values
      blockchain_token->chain_id = chain_id;
      blockchain_token->coin = coin;
      blockchain_token->visible = true;
      blockchain_token->is_erc20 =
          coin == mojom::CoinType::ETH && !blockchain_token->is_nft;
      blockchain_token->is_erc1155 = false;
      blockchain_token->spl_token_program =
          IsSPLToken(blockchain_token)
              ? (is_token2022 ? mojom::SPLTokenProgram::kToken2022
                              : mojom::SPLTokenProgram::kToken)
              : mojom::SPLTokenProgram::kUnsupported;

      (*token_list_map)[GetTokenListKey(coin, chain_id)].push_back(
          std::move(blockchain_token));
    }
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
    return false;
  }

  const base::ListValue* chain_list = records_v->GetIfList();
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
    network->chain_id = absl::StrFormat("0x%x", chain_id);

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
    network->supported_keyrings = {mojom::KeyringId::kDefault};

    result->push_back(std::move(network));
  }

  return true;
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

  std::optional<base::DictValue> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  std::map<std::pair<std::string, std::string>, std::string> coingecko_ids_map;
  for (const auto chain_id_record : *records_v) {
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

std::optional<std::vector<std::string>> ParseRestrictedAddressesList(
    const std::string& json) {
  // {
  //   "addresses": [
  //     "t1MMXtBrSp1XG38Lx9cePcNUCJj5vdWfUWL",
  //     "t1WSKwCDL1QYRRUrCCknEs5tDLhtGVYu9KM",
  //     ...
  //   ]
  // }
  std::optional<base::DictValue> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  auto restricted_list_from_component =
      blockchain_lists::RestrictedAddressesList::FromValue(*records_v);
  if (!restricted_list_from_component) {
    return std::nullopt;
  }

  std::vector<std::string> restricted_list;
  for (const auto& address : (*restricted_list_from_component).addresses) {
    restricted_list.push_back(base::ToLowerASCII(address));
  }

  return restricted_list;
}

std::optional<PolkadotRpcUrlsMap> ParsePolkadotRpcUrls(
    const std::string& json) {
  // {
  //   "polkadot_acala": ["https://acala-rpc-0.aca-api.network"],
  //   "kusama_asset_hub": ["https://rpc-asset-hub-kusama.luckyfriday.io"],
  //   ...
  // }
  std::optional<base::DictValue> records_v = ParseJsonToDict(json);
  if (!records_v) {
    return std::nullopt;
  }

  PolkadotRpcUrlsMap rpc_urls_map;
  for (const auto chain_record : *records_v) {
    const auto* urls = chain_record.second.GetIfList();
    if (!urls) {
      return std::nullopt;
    }

    std::vector<GURL> chain_urls;
    for (const auto& url_value : *urls) {
      const auto* url_string = url_value.GetIfString();
      if (!url_string) {
        return std::nullopt;
      }

      GURL url(*url_string);
      if (!url.is_valid()) {
        return std::nullopt;
      }

      chain_urls.push_back(std::move(url));
    }

    rpc_urls_map[chain_record.first] = std::move(chain_urls);
  }

  return rpc_urls_map;
}

}  // namespace brave_wallet
