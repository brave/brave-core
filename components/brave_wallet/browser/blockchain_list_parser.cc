/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace {

constexpr char kDappRadarSolanaId[] = "solana";
constexpr char kDappRadarEthereumId[] = "ethereum";
constexpr char kDappRadarPolygonId[] = "polygon";
constexpr char kDappRadarBinanceSmartChainId[] = "binance-smart-chain";
constexpr char kDappRadarOptimismId[] = "optimism";
constexpr char kDappRadarAuroraId[] = "aurora";
constexpr char kDappRadarAvalancheId[] = "avalanche";
constexpr char kDappRadarFantomId[] = "fantom";

absl::optional<std::tuple<brave_wallet::mojom::CoinType, std::string>>
DappRadarChainIdToCoinAndChainId(const std::string& dapp_radar_chain_id) {
  static base::NoDestructor<base::flat_map<
      std::string, std::tuple<brave_wallet::mojom::CoinType, std::string>>>
      chain_id_lookup({
          {kDappRadarEthereumId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kMainnetChainId}},
          {kDappRadarSolanaId,
           {brave_wallet::mojom::CoinType::SOL,
            brave_wallet::mojom::kSolanaMainnet}},
          {kDappRadarPolygonId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kPolygonMainnetChainId}},
          {kDappRadarBinanceSmartChainId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kBinanceSmartChainMainnetChainId}},
          {kDappRadarOptimismId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kOptimismMainnetChainId}},
          {kDappRadarAuroraId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kArbitrumMainnetChainId}},
          {kDappRadarAvalancheId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kAvalancheMainnetChainId}},
          {kDappRadarFantomId,
           {brave_wallet::mojom::CoinType::ETH,
            brave_wallet::mojom::kFantomMainnetChainId}},
      });
  if (!chain_id_lookup->contains(dapp_radar_chain_id)) {
    return absl::nullopt;
  }

  return chain_id_lookup->at(dapp_radar_chain_id);
}

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

bool ParseIntResultFromDict(const base::Value::Dict* response_dict,
                            const std::string& key,
                            uint32_t* output_val) {
  auto val = response_dict->FindInt(key);
  if (!val) {
    return false;
  }
  *output_val = *val;
  return true;
}

bool ParseDoubleResultFromDict(const base::Value::Dict* response_dict,
                               const std::string& key,
                               double* output_val) {
  auto val = response_dict->FindDouble(key);
  if (!val) {
    return false;
  }
  *output_val = *val;
  return true;
}

std::string EmptyIfNull(const std::string* str) {
  if (str) {
    return *str;
  }
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
    if (is_erc20_opt) {
      blockchain_token->is_erc20 = *is_erc20_opt;
    } else {
      blockchain_token->is_erc20 = false;
    }

    absl::optional<bool> is_erc721_opt =
        blockchain_token_value->FindBool("erc721");
    if (is_erc721_opt) {
      blockchain_token->is_erc721 = *is_erc721_opt;
    } else {
      blockchain_token->is_erc721 = false;
    }

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

    absl::optional<int> decimals_opt =
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
            if (GURL(*url).is_valid()) {
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
          if (GURL(*url).is_valid()) {
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

    result->push_back(std::move(network));
  }

  return true;
}

bool ParseDappLists(const std::string& json, DappListMap* dapp_list) {
  DCHECK(dapp_list);

  // {
  //   "ethereum": {
  //     "success": true,
  //     "chain": "ethereum",
  //     "category": null,
  //     "range": "30d",
  //     "top": 10,
  //     "results": [
  //       {
  //         "dappId": 7000,
  //         "name": "Uniswap V3",
  //         "description": "A protocol for trading and automated liquidity
  //         provision on Ethereum.", "fullDescription": "<p>Uniswap v3
  //         introduces:</p>\n<ul>\n  <li><strong>Concentrated
  //         liquidity,</strong> giving individual LPs granular control over
  //         what price ranges their capital is allocated to. Individual
  //         positions are aggregated together into a single pool, forming one
  //         combined curve for users to trade against</li>\n
  //         <li><strong>Multiple fee tiers</strong> , allowing LPs to be
  //         appropriately compensated for taking on varying degrees of
  //         risk</li>\n</ul>\n<p>These features make Uniswap v3 <strong>the
  //         most flexible and efficient AMM ever designed</strong>:</p>\n<ul>\n
  //         <li>LPs can provide liquidity with <strong>up to 4000x capital
  //         efficiency</strong> relative to Uniswap v2, earning <strong>higher
  //         returns on their capital</strong></li>\n  <li>Capital efficiency
  //         paves the way for low-slippage <strong>trade execution that can
  //         surpass both centralized exchanges and stablecoin-focused
  //         AMMs</strong></li>\n  <li>LPs can significantly <strong>increase
  //         their exposure to preferred assets</strong> and <strong>reduce
  //         their downside risk</strong></li>\n  <li>LPs can sell one asset for
  //         another by adding liquidity to a price range entirely above or
  //         below the market price, approximating <strong>a fee-earning limit
  //         order that executes along a smooth curve</strong></li>\n</ul>",
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
  //         "socialLinks": [
  //           {
  //             "title": "blog",
  //             "url": "https://uniswap.org/blog/",
  //             "type": "blog"
  //           },
  //           {
  //             "title": "discord",
  //             "url": "https://discord.com/invite/FCfyBSbCU5",
  //             "type": "discord"
  //           },
  //           {
  //             "title": "github",
  //             "url": "https://github.com/Uniswap",
  //             "type": "github"
  //           },
  //           {
  //             "title": "reddit",
  //             "url": "https://www.reddit.com/r/UniSwap/",
  //             "type": "reddit"
  //           },
  //           {
  //             "title": "twitter",
  //             "url": "https://twitter.com/Uniswap",
  //             "type": "twitter"
  //           }
  //         ],
  //         "metrics": {
  //           "transactions": 2348167,
  //           "uaw": 387445,
  //           "volume": 65982226285.39,
  //           "balance": 1904817795.53
  //         }
  //       }
  //     ]
  //   },
  //   "solana": {
  //     "success": true,
  //     "chain": "solana",
  //     "category": null,
  //     "range": "30d",
  //     "top": 10,
  //     "results": [
  //       {
  //         "dappId": 20419,
  //         "name": "GameTrade Market",
  //         "description": "Discover, buy, sell and trade in-game NFTs",
  //         "fullDescription": "<p><strong>GameTrade Market is an easy-to-use
  //         Web3 gaming marketplace and social network for
  //         gamers.</strong></p>\n<p>- <strong>Social media
  //         tools</strong></p>\n<p>Messaging, news feed, referral programs.
  //         Great networking capabilities for finding friends and
  //         clients.</p>\n<p>- <strong>Game database</strong> For each game,
  //         there is a detailed description along with screenshots, gameplay
  //         videos, community reviews.</p>\n<p>- <strong>Multiple blockchain
  //         support</strong></p>\n<p>Games and tokens based on dozens of
  //         different blockchains.</p>\n<p>- <strong>Custom game
  //         coins</strong></p>\n<p>Exchange in-game currencies and native
  //         blockchain coins on the built-in crypto exchange.</p>\n<p>-
  //         <strong>Swap and Rent</strong></p>\n<p>New possibilities for the
  //         in-game NFT economies: swap and rent.</p>\n<p>-
  //         <strong>Community</strong></p>\n<p>User reputation, game reviews,
  //         item comments, user-generated game guides .</p>", "logo":
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
  //         "socialLinks": [
  //           {
  //             "title": "discord",
  //             "url": "https://discord.gg/gametrade",
  //             "type": "discord"
  //           },
  //           {
  //             "title": "medium",
  //             "url": "https://gametrademarket.medium.com/",
  //             "type": "medium"
  //           },
  //           {
  //             "title": "twitter",
  //             "url": "https://twitter.com/GameTradeMarket",
  //             "type": "twitter"
  //           },
  //           {
  //             "title": "youtube",
  //             "url":
  //             "https://www.youtube.com/channel/UCAoMHO4zQaiT-vxWOVk8IjA/videos",
  //             "type": "youtube"
  //           }
  //         ],
  //         "metrics": {
  //           "transactions": 401926,
  //           "uaw": 354495,
  //           "volume": 8949.83,
  //           "balance": 3.81
  //         }
  //       }
  //     ]
  //   }
  // }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto& response_dict = records_v->GetDict();
  for (const auto chain_dapp_list_pair : response_dict) {
    const auto& dapp_radar_chain_id = chain_dapp_list_pair.first;
    const auto* dapp_list_value = chain_dapp_list_pair.second.GetIfDict();
    if (!dapp_list_value) {
      return false;
    }

    if (auto* results_list = dapp_list_value->FindList("results")) {
      for (auto& item : *results_list) {
        auto dapp = mojom::Dapp::New();
        if (auto* dappDict = item.GetIfDict()) {
          if (!ParseIntResultFromDict(dappDict, "dappId", &dapp->id) ||
              !ParseResultFromDict(dappDict, "name", &dapp->name) ||
              !ParseResultFromDict(dappDict, "description",
                                   &dapp->description) ||
              !ParseResultFromDict(dappDict, "logo", &dapp->logo) ||
              !ParseResultFromDict(dappDict, "website", &dapp->website)) {
            continue;
          }

          // Parse chains
          if (auto* chains_list = dappDict->FindList("chains")) {
            for (const auto& chain : *chains_list) {
              if (chain.is_string()) {
                dapp->chains.push_back(chain.GetString());
              }
            }
          }

          // Parse categories
          if (auto* categories_list = dappDict->FindList("categories")) {
            for (const auto& category : *categories_list) {
              if (category.is_string()) {
                dapp->categories.push_back(category.GetString());
              }
            }
          }

          // Parse metrics
          if (auto* metrics_dict = dappDict->FindDict("metrics")) {
            if (!ParseIntResultFromDict(metrics_dict, "transactions",
                                        &dapp->transactions) ||
                !ParseIntResultFromDict(metrics_dict, "uaw", &dapp->uaw) ||
                !ParseDoubleResultFromDict(metrics_dict, "volume",
                                           &dapp->volume) ||
                !ParseDoubleResultFromDict(metrics_dict, "balance",
                                           &dapp->balance)) {
              continue;
            }
          }

          // Determine the correct key, then add parsed dapp to the dapp_list
          absl::optional<std::tuple<mojom::CoinType, std::string>>
              coin_and_chain_id_result =
                  DappRadarChainIdToCoinAndChainId(dapp_radar_chain_id);
          if (!coin_and_chain_id_result) {
            continue;
          }
          mojom::CoinType coin = std::get<0>(*coin_and_chain_id_result);
          std::string chain_id = std::get<1>(*coin_and_chain_id_result);
          std::string key = GetTokenListKey(coin, chain_id);
          (*dapp_list)[key].push_back(std::move(dapp));
        }
      }
    }
  }

  return true;
}

}  // namespace brave_wallet
