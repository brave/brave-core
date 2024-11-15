/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAreArray;

namespace brave_wallet {

TEST(BlockchainListParseUnitTest, ParseTokenList) {
  std::string json(R"(
    {
     "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
       "name": "Crypto Kitties",
       "logo": "CryptoKitties-Kitty-13733.svg",
       "erc20": false,
       "erc721": true,
       "symbol": "CK",
       "decimals": 0
     },
     "0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {
       "name": "Basic Attention Token",
       "logo": "bat.svg",
       "erc20": true,
       "symbol": "BAT",
       "decimals": 18,
       "coingeckoId": "basic-attention-token"
     },
     "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
       "name": "Uniswap",
       "logo": "uni.svg",
       "erc20": true,
       "symbol": "UNI",
       "decimals": 18,
       "chainId": "0xaa36a7"
     }
    }
  )");

  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  ASSERT_EQ(token_list_map["ethereum.0x1"].size(), 2UL);
  EXPECT_EQ(token_list_map["ethereum.0x2"].size(), 0UL);
  ASSERT_EQ(token_list_map["ethereum.0xaa36a7"].size(), 1UL);

  const auto& mainnet_token_list = token_list_map["ethereum.0x1"];
  EXPECT_EQ(mainnet_token_list[0]->name, "Crypto Kitties");
  EXPECT_EQ(mainnet_token_list[0]->contract_address,
            "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
  EXPECT_FALSE(mainnet_token_list[0]->is_erc20);
  EXPECT_TRUE(mainnet_token_list[0]->is_erc721);
  EXPECT_TRUE(mainnet_token_list[0]->is_nft);
  EXPECT_EQ(mainnet_token_list[0]->symbol, "CK");
  EXPECT_EQ(mainnet_token_list[0]->logo, "CryptoKitties-Kitty-13733.svg");
  EXPECT_EQ(mainnet_token_list[0]->decimals, 0);
  EXPECT_TRUE(mainnet_token_list[0]->coingecko_id.empty());
  EXPECT_EQ(mainnet_token_list[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

  EXPECT_EQ(mainnet_token_list[1]->name, "Basic Attention Token");
  EXPECT_EQ(mainnet_token_list[1]->contract_address,
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_TRUE(mainnet_token_list[1]->is_erc20);
  EXPECT_FALSE(mainnet_token_list[1]->is_erc721);
  EXPECT_FALSE(mainnet_token_list[1]->is_erc1155);
  EXPECT_FALSE(mainnet_token_list[1]->is_nft);
  EXPECT_EQ(mainnet_token_list[1]->symbol, "BAT");
  EXPECT_EQ(mainnet_token_list[1]->logo, "bat.svg");
  EXPECT_EQ(mainnet_token_list[1]->decimals, 18);
  EXPECT_EQ(mainnet_token_list[1]->coingecko_id, "basic-attention-token");
  EXPECT_EQ(mainnet_token_list[1]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

  const auto& sepolia_token_list = token_list_map["ethereum.0xaa36a7"];
  EXPECT_EQ(sepolia_token_list[0]->name, "Uniswap");
  EXPECT_EQ(sepolia_token_list[0]->contract_address,
            "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
  EXPECT_TRUE(sepolia_token_list[0]->is_erc20);
  EXPECT_FALSE(sepolia_token_list[0]->is_erc721);
  EXPECT_FALSE(sepolia_token_list[0]->is_erc1155);
  EXPECT_FALSE(sepolia_token_list[0]->is_nft);
  EXPECT_EQ(sepolia_token_list[0]->symbol, "UNI");
  EXPECT_EQ(sepolia_token_list[0]->logo, "uni.svg");
  EXPECT_EQ(sepolia_token_list[0]->decimals, 18);
  EXPECT_TRUE(sepolia_token_list[0]->coingecko_id.empty());
  EXPECT_EQ(sepolia_token_list[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

  std::string solana_json(R"(
    {
      "So11111111111111111111111111111111111111112": {
        "name": "Wrapped SOL",
        "logo": "So11111111111111111111111111111111111111112.png",
        "erc20": false,
        "symbol": "SOL",
        "decimals": 9,
        "chainId": "0x65",
        "coingeckoId": "solana"
      },
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v": {
        "name": "USD Coin",
        "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
        "erc20": false,
        "symbol": "USDC",
        "decimals": 6,
        "chainId": "0x65",
        "coingeckoId": "usd-coin"
      },
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ": {
        "name": "Tesla Inc.",
        "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
        "erc20": false,
        "token2022": true,
        "symbol": "TSLA",
        "decimals": 8,
        "chainId": "0x65"
      }
    }
  )");
  EXPECT_TRUE(
      ParseTokenList(solana_json, &token_list_map, mojom::CoinType::SOL));
  auto wrapped_sol = mojom::BlockchainToken::New(
      "So11111111111111111111111111111111111111112", "Wrapped SOL",
      "So11111111111111111111111111111111111111112.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "SOL", 9, true, "",
      "solana", "0x65", mojom::CoinType::SOL, false);
  auto usdc = mojom::BlockchainToken::New(
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", "USD Coin",
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "USDC", 6, true, "",
      "usd-coin", "0x65", mojom::CoinType::SOL, false);
  auto tsla = mojom::BlockchainToken::New(
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "Tesla Inc.",
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken2022, false, false, "TSLA", 8, true,
      "", "", "0x65", mojom::CoinType::SOL, false);
  std::vector<mojom::BlockchainTokenPtr> solana_token_list;
  solana_token_list.push_back(std::move(tsla));
  solana_token_list.push_back(std::move(usdc));
  solana_token_list.push_back(std::move(wrapped_sol));
  EXPECT_EQ(token_list_map["solana.0x65"], solana_token_list);

  token_list_map.clear();
  json = R"({})";
  EXPECT_TRUE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  EXPECT_TRUE(token_list_map.empty());
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": 3})";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {}})";
  EXPECT_TRUE(token_list_map.empty());
  EXPECT_TRUE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  json = "3";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  json = "[3]";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  json = "";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
}

TEST(ParseTokenListUnitTest, GetTokenListKey) {
  EXPECT_EQ(GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId),
            "ethereum.0x1");
  EXPECT_EQ(GetTokenListKey(mojom::CoinType::FIL, mojom::kFilecoinMainnet),
            "filecoin.f");
  EXPECT_EQ(GetTokenListKey(mojom::CoinType::SOL, mojom::kSolanaMainnet),
            "solana.0x65");
}

TEST(BlockchainListParseUnitTest, ParseChainList) {
  const std::string chain_list = R"(
  [
    {
      "name": "Ethereum Mainnet",
      "chain": "ETH",
      "icon": "ethereum",
      "rpc": [
        "https://mainnet.infura.io/v3/${INFURA_API_KEY}",
        "wss://mainnet.infura.io/ws/v3/${INFURA_API_KEY}",
        "http://api.com/eth",
        "http://127.0.0.1:5566/eth",
        "https://api.mycryptoapi.com/eth",
        "https://cloudflare-eth.com"
      ],
      "faucets": [],
      "nativeCurrency": { "name": "Ether", "symbol": "ETH", "decimals": 18 },
      "infoURL": "https://ethereum.org",
      "shortName": "eth",
      "chainId": 1,
      "networkId": 1,
      "slip44": 60,
      "ens": { "registry": "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e" },
      "explorers": [
        {
          "name": "etherscan",
          "url": "https://etherscan.io",
          "standard": "EIP3091"
        },
        {
          "name": "invalid http",
          "url": "http://test.com",
          "standard": "EIP3091"
        },
        {
          "name": "localhost",
          "url": "http://localhost:8080",
          "standard": "EIP3091"
        }
      ]
    },
    {
      "name": "Polygon Mainnet",
      "chain": "Polygon",
      "rpc": [
        "https://polygon-rpc.com/",
        "https://rpc-mainnet.matic.network",
        "https://matic-mainnet.chainstacklabs.com",
        "https://rpc-mainnet.maticvigil.com",
        "https://rpc-mainnet.matic.quiknode.pro",
        "https://matic-mainnet-full-rpc.bwarelabs.com"
      ],
      "faucets": [],
      "nativeCurrency": { "name": "MATIC", "symbol": "MATIC", "decimals": 18 },
      "infoURL": "https://polygon.technology/",
      "shortName": "MATIC",
      "chainId": 137,
      "networkId": 137,
      "slip44": 966,
      "explorers": [
        {
          "name": "polygonscan",
          "url": "https://polygonscan.com",
          "standard": "EIP3091"
        }
      ]
    }
  ])";

  ChainList result;
  EXPECT_TRUE(ParseChainList(chain_list, &result));

  ASSERT_EQ(2u, result.size());

  auto& chain1 = result[0];
  ASSERT_TRUE(chain1);
  EXPECT_EQ("0x1", chain1->chain_id);
  EXPECT_EQ("Ethereum Mainnet", chain1->chain_name);
  EXPECT_THAT(
      chain1->rpc_endpoints,
      ElementsAreArray({GURL("https://mainnet.infura.io/v3/${INFURA_API_KEY}"),
                        GURL("http://127.0.0.1:5566/eth"),
                        GURL("https://api.mycryptoapi.com/eth"),
                        GURL("https://cloudflare-eth.com")}));
  EXPECT_EQ(1, chain1->active_rpc_endpoint_index);
  EXPECT_THAT(
      chain1->block_explorer_urls,
      ElementsAreArray({"https://etherscan.io", "http://localhost:8080"}));
  EXPECT_EQ("Ether", chain1->symbol_name);
  EXPECT_EQ("ETH", chain1->symbol);
  EXPECT_EQ(18, chain1->decimals);
  EXPECT_EQ(0u, chain1->icon_urls.size());
  EXPECT_EQ(chain1->coin, mojom::CoinType::ETH);

  auto& chain2 = result[1];
  ASSERT_TRUE(chain2);
  EXPECT_EQ("0x89", chain2->chain_id);
  EXPECT_EQ("Polygon Mainnet", chain2->chain_name);
  EXPECT_THAT(
      chain2->rpc_endpoints,
      ElementsAreArray({GURL("https://polygon-rpc.com/"),
                        GURL("https://rpc-mainnet.matic.network"),
                        GURL("https://matic-mainnet.chainstacklabs.com"),
                        GURL("https://rpc-mainnet.maticvigil.com"),
                        GURL("https://rpc-mainnet.matic.quiknode.pro"),
                        GURL("https://matic-mainnet-full-rpc.bwarelabs.com")}));
  EXPECT_EQ(0, chain2->active_rpc_endpoint_index);
  EXPECT_THAT(chain2->block_explorer_urls,
              ElementsAreArray({"https://polygonscan.com"}));
  EXPECT_EQ("MATIC", chain2->symbol_name);
  EXPECT_EQ("MATIC", chain2->symbol);
  EXPECT_EQ(18, chain2->decimals);
  EXPECT_EQ(0u, chain2->icon_urls.size());
  EXPECT_EQ(chain2->coin, mojom::CoinType::ETH);
}

TEST(BlockchainListParseUnitTest, ParseDappLists) {
  const std::string dapp_list = R"({
    "solana": {
      "success": true,
      "chain": "solana",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": [
        {
          "dappId": "20419",
          "name": "GameTrade Market",
          "description": "Discover, buy, sell and trade in-game NFTs",
          "logo": "https://dashboard-assets.dappradar.com/document/20419/gametrademarket-dapp-marketplaces-matic-logo_e3e698e60ebd9bfe8ed1421bb41b890d.png",
          "link": "https://dappradar.com/solana/marketplaces/gametrade-market-2",
          "website": "https://gametrade.market/",
          "chains": [
            "polygon",
            "solana",
            "binance-smart-chain"
          ],
          "categories": [
            "marketplaces"
          ],
          "metrics": {
            "transactions": "1513120",
            "uaw": "917737",
            "volume": "32352.38",
            "balance": "3.81"
          }
        }
      ]
    },
    "ethereum": {
      "success": true,
      "chain": "ethereum",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": [
        {
          "dappId": "7000",
          "name": "Uniswap V3",
          "description": "A protocol for trading and automated liquidity.",
          "logo": "https://dashboard-assets.dappradar.com/document/7000/uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png",
          "link": "https://dappradar.com/ethereum/exchanges/uniswap-v3",
          "website": "https://app.uniswap.org/#/swap",
          "chains": [
            "ethereum",
            "polygon",
            "optimism",
            "celo",
            "arbitrum",
            "binance-smart-chain"
          ],
          "categories": [
            "exchanges"
          ],
          "metrics": {
            "transactions": "3596443",
            "uaw": "507730",
            "volume": "42672855706.52",
            "balance": "1887202135.14"
          }
        }
      ]
    },
    "polygon": {
      "success": true,
      "chain": "polygon",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": [
        {
          "dappId": "18305",
          "name": "Stargate",
          "description": "A Composable Omnichain Native Asset Bridge",
          "logo": "https://dashboard-assets.dappradar.com/document/18305/stargatefinance-dapp-defi-ethereum-logo_66dc9532020488c50870b5dae3e34654.png",
          "link": "https://dappradar.com/polygon/defi/stargate-2",
          "website": "https://stargate.finance/",
          "chains": [
            "ethereum",
            "binance-smart-chain",
            "avalanche",
            "polygon",
            "optimism",
            "fantom",
            "arbitrum"
          ],
          "categories": [
            "defi"
          ],
          "metrics": {
            "transactions": null,
            "uaw": null,
            "volume": null,
            "balance": null
          }
        }
      ]
    },
    "binance_smart_chain": {
      "success": true,
      "chain": "binance-smart-chain",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": []
    },
    "optimism": {
      "success": true,
      "chain": "optimism",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": []
    },
    "aurora": {
      "success": true,
      "chain": "aurora",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": []
    },
    "avalanche": {
      "success": true,
      "chain": "avalanche",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": []
    },
    "fantom": {
      "success": true,
      "chain": "fantom",
      "category": null,
      "range": "30d",
      "top": "100",
      "results": []
    }
  })";

  // Parse the dapp list
  std::optional<DappListMap> dapp_list_map = ParseDappLists(dapp_list);
  ASSERT_TRUE(dapp_list_map);

  // There should be eight lists, for Ethereum, Solana, Polygon, Binance Smart
  // Chain, Optimism, Aurora, Avalanche, and Fantom
  ASSERT_EQ(8u, dapp_list_map->size());

  // There should be one dapp in the Ethereum list
  auto it = dapp_list_map->find(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId));
  EXPECT_TRUE(it != dapp_list_map->end());
  const auto& eth_dapp_list = it->second;
  EXPECT_EQ(eth_dapp_list.size(), 1u);
  const auto& eth_dapp = eth_dapp_list[0];
  EXPECT_EQ(eth_dapp->range, "30d");
  EXPECT_EQ(eth_dapp->name, "Uniswap V3");
  EXPECT_EQ(eth_dapp->description,
            "A protocol for trading and automated liquidity.");
  EXPECT_EQ(
      eth_dapp->logo,
      "https://dashboard-assets.dappradar.com/document/7000/"
      "uniswapv3-dapp-defi-ethereum-logo_7f71f0c5a1cd26a3e3ffb9e8fb21b26b.png");
  EXPECT_EQ(eth_dapp->website, "https://app.uniswap.org/#/swap");
  EXPECT_EQ(eth_dapp->categories.size(), 1u);
  EXPECT_EQ(eth_dapp->categories[0], "exchanges");
  EXPECT_EQ(eth_dapp->chains.size(), 6u);
  EXPECT_EQ(eth_dapp->chains[0], "ethereum");
  EXPECT_EQ(eth_dapp->chains[1], "polygon");
  EXPECT_EQ(eth_dapp->chains[2], "optimism");
  EXPECT_EQ(eth_dapp->chains[3], "celo");
  EXPECT_EQ(eth_dapp->chains[4], "arbitrum");
  EXPECT_EQ(eth_dapp->chains[5], "binance-smart-chain");
  EXPECT_EQ(eth_dapp->transactions, 3596443u);
  EXPECT_EQ(eth_dapp->uaw, 507730u);
  EXPECT_DOUBLE_EQ(eth_dapp->volume, 42672855706.52);
  EXPECT_DOUBLE_EQ(eth_dapp->balance, 1887202135.14);

  // There should be one dapp in the Solana list
  auto it_s = dapp_list_map->find(
      GetTokenListKey(mojom::CoinType::SOL, mojom::kSolanaMainnet));
  EXPECT_TRUE(it_s != dapp_list_map->end());
  const auto& sol_dapp_list = it_s->second;
  EXPECT_EQ(sol_dapp_list.size(), 1u);
  const auto& sol_dapp = sol_dapp_list[0];
  EXPECT_EQ(sol_dapp->range, "30d");
  EXPECT_EQ(sol_dapp->name, "GameTrade Market");
  EXPECT_EQ(sol_dapp->description,
            "Discover, buy, sell and trade in-game NFTs");
  EXPECT_EQ(sol_dapp->logo,
            "https://dashboard-assets.dappradar.com/document/20419/"
            "gametrademarket-dapp-marketplaces-matic-logo_"
            "e3e698e60ebd9bfe8ed1421bb41b890d.png");

  EXPECT_EQ(sol_dapp->website, "https://gametrade.market/");
  EXPECT_EQ(sol_dapp->categories.size(), 1u);
  EXPECT_EQ(sol_dapp->categories[0], "marketplaces");
  EXPECT_EQ(sol_dapp->chains.size(), 3u);
  EXPECT_EQ(sol_dapp->chains[0], "polygon");
  EXPECT_EQ(sol_dapp->chains[1], "solana");
  EXPECT_EQ(sol_dapp->chains[2], "binance-smart-chain");
  EXPECT_EQ(sol_dapp->transactions, 1513120u);
  EXPECT_EQ(sol_dapp->uaw, 917737u);
  EXPECT_DOUBLE_EQ(sol_dapp->volume, 32352.38);
  EXPECT_DOUBLE_EQ(sol_dapp->balance, 3.81);

  // There should be no dapps in the Polygon list because the only
  // dapp in that list had null values for metrics.
  auto it_p = dapp_list_map->find(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kPolygonMainnetChainId));
  EXPECT_TRUE(it_p != dapp_list_map->end());
  const auto& poly_dapp_list = it_p->second;
  EXPECT_EQ(poly_dapp_list.size(), 0u);
}

TEST(BlockchainListParseUnitTest, ParseOnRampTokensListMap) {
  // Invalid JSON is not parsed
  std::optional<RampTokenListMaps> supported_tokens_list_map =
      ParseRampTokenListMaps(R"({)");
  ASSERT_FALSE(supported_tokens_list_map);

  // Valid JSON is parsed into supported token list map
  const std::string supported_tokens_list = R"({
    "tokens" : [
       {
         "chain_id": "0x1",
         "coin": 60,
         "coingecko_id": "",
         "contract_address": "",
         "decimals": 18,
         "is_erc1155": false,
         "is_erc20": false,
         "is_erc721": false,
         "is_nft": false,
         "logo": "",
         "name": "Ethereum",
         "symbol": "ETH",
         "token_id": "",
         "visible": true,
         "on_ramp_providers": ["ramp", "sardine", "transak", "stripe"],
         "off_ramp_providers": []
       },
       {
         "chain_id": "0x38",
         "coin": 60,
         "coingecko_id": "",
         "contract_address": "",
         "decimals": 18,
         "is_erc1155": false,
         "is_erc20": true,
         "is_erc721": false,
         "is_nft": false,
         "logo": "",
         "name": "BNB",
         "symbol": "BNB",
         "token_id": "",
         "visible": true,
         "on_ramp_providers": ["ramp"],
         "off_ramp_providers": []
       },
       {
         "chain_id": "0x1",
         "coin": 60,
         "coingecko_id": "",
         "contract_address": "0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9",
         "decimals": 18,
         "is_erc1155": false,
         "is_erc20": true,
         "is_erc721": false,
         "is_nft": false,
         "logo": "aave.png",
         "name": "AAVE",
         "symbol": "AAVE",
         "token_id": "",
         "visible": true,
         "on_ramp_providers": ["sardine"],
         "off_ramp_providers": []
       },
       {
         "chain_id": "0xa",
         "coin": 60,
         "coingecko_id": "",
         "contract_address": "",
         "decimals": 18,
         "is_erc1155": false,
         "is_erc20": false,
         "is_erc721": false,
         "is_nft": false,
         "logo": "",
         "name": "Ethereum",
         "symbol": "ETH",
         "token_id": "",
         "visible": true,
         "on_ramp_providers": ["transak"],
         "off_ramp_providers": []
       }
    ]
  })";

  supported_tokens_list_map = ParseRampTokenListMaps(supported_tokens_list);
  ASSERT_TRUE(supported_tokens_list_map);
  EXPECT_EQ((*supported_tokens_list_map).first.size(), 4UL);

  auto it =
      (*supported_tokens_list_map).first.find(mojom::OnRampProvider::kRamp);
  ASSERT_NE(it, (*supported_tokens_list_map).first.end());
  EXPECT_EQ(it->second.size(), 2UL);
  EXPECT_EQ(it->second[0]->contract_address, "");
  EXPECT_EQ(it->second[0]->name, "Ethereum");
  EXPECT_EQ(it->second[0]->logo, "");
  EXPECT_EQ(it->second[0]->is_erc20, false);
  EXPECT_EQ(it->second[0]->is_erc721, false);
  EXPECT_EQ(it->second[0]->is_erc1155, false);
  EXPECT_EQ(it->second[0]->is_nft, false);
  EXPECT_EQ(it->second[0]->symbol, "ETH");
  EXPECT_EQ(it->second[0]->decimals, 18);
  EXPECT_EQ(it->second[0]->visible, true);
  EXPECT_EQ(it->second[0]->token_id, "");
  EXPECT_EQ(it->second[0]->coingecko_id, "");
  EXPECT_EQ(it->second[0]->chain_id, "0x1");
  EXPECT_EQ(it->second[0]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(it->second[1]->contract_address, "");
  EXPECT_EQ(it->second[1]->name, "BNB");
  EXPECT_EQ(it->second[1]->logo, "");
  EXPECT_EQ(it->second[1]->is_erc20, true);
  EXPECT_EQ(it->second[1]->is_erc721, false);
  EXPECT_EQ(it->second[1]->is_erc1155, false);
  EXPECT_EQ(it->second[1]->is_nft, false);
  EXPECT_EQ(it->second[1]->symbol, "BNB");
  EXPECT_EQ(it->second[1]->decimals, 18);
  EXPECT_EQ(it->second[1]->visible, true);
  EXPECT_EQ(it->second[1]->token_id, "");
  EXPECT_EQ(it->second[1]->coingecko_id, "");
  EXPECT_EQ(it->second[1]->chain_id, "0x38");
  EXPECT_EQ(it->second[1]->coin, mojom::CoinType::ETH);

  it = (*supported_tokens_list_map).first.find(mojom::OnRampProvider::kSardine);
  ASSERT_NE(it, (*supported_tokens_list_map).first.end());
  EXPECT_EQ(it->second.size(), 2UL);
  EXPECT_EQ(it->second[0]->contract_address, "");
  EXPECT_EQ(it->second[0]->name, "Ethereum");
  EXPECT_EQ(it->second[0]->logo, "");
  EXPECT_EQ(it->second[0]->is_erc20, false);
  EXPECT_EQ(it->second[0]->is_erc721, false);
  EXPECT_EQ(it->second[0]->is_erc1155, false);
  EXPECT_EQ(it->second[0]->is_nft, false);
  EXPECT_EQ(it->second[0]->symbol, "ETH");
  EXPECT_EQ(it->second[0]->decimals, 18);
  EXPECT_EQ(it->second[0]->visible, true);
  EXPECT_EQ(it->second[0]->token_id, "");
  EXPECT_EQ(it->second[0]->coingecko_id, "");
  EXPECT_EQ(it->second[0]->chain_id, "0x1");
  EXPECT_EQ(it->second[0]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(it->second[1]->contract_address,
            "0x7Fc66500c84A76Ad7e9c93437bFc5Ac33E2DDaE9");
  EXPECT_EQ(it->second[1]->name, "AAVE");
  EXPECT_EQ(it->second[1]->logo, "aave.png");
  EXPECT_EQ(it->second[1]->is_erc20, true);
  EXPECT_EQ(it->second[1]->is_erc721, false);
  EXPECT_EQ(it->second[1]->is_erc1155, false);
  EXPECT_EQ(it->second[1]->is_nft, false);
  EXPECT_EQ(it->second[1]->symbol, "AAVE");
  EXPECT_EQ(it->second[1]->decimals, 18);
  EXPECT_EQ(it->second[1]->visible, true);
  EXPECT_EQ(it->second[1]->token_id, "");
  EXPECT_EQ(it->second[1]->coingecko_id, "");
  EXPECT_EQ(it->second[1]->chain_id, "0x1");
  EXPECT_EQ(it->second[1]->coin, mojom::CoinType::ETH);

  it = (*supported_tokens_list_map).first.find(mojom::OnRampProvider::kTransak);
  ASSERT_NE(it, (*supported_tokens_list_map).first.end());
  EXPECT_EQ(it->second.size(), 2UL);
  EXPECT_EQ(it->second[0]->contract_address, "");
  EXPECT_EQ(it->second[0]->name, "Ethereum");
  EXPECT_EQ(it->second[0]->logo, "");
  EXPECT_EQ(it->second[0]->is_erc20, false);
  EXPECT_EQ(it->second[0]->is_erc721, false);
  EXPECT_EQ(it->second[0]->is_erc1155, false);
  EXPECT_EQ(it->second[0]->is_nft, false);
  EXPECT_EQ(it->second[0]->symbol, "ETH");
  EXPECT_EQ(it->second[0]->decimals, 18);
  EXPECT_EQ(it->second[0]->visible, true);
  EXPECT_EQ(it->second[0]->token_id, "");
  EXPECT_EQ(it->second[0]->coingecko_id, "");
  EXPECT_EQ(it->second[0]->chain_id, "0x1");
  EXPECT_EQ(it->second[0]->coin, mojom::CoinType::ETH);
  EXPECT_EQ(it->second[1]->contract_address, "");
  EXPECT_EQ(it->second[1]->name, "Ethereum");
  EXPECT_EQ(it->second[1]->logo, "");
  EXPECT_EQ(it->second[1]->is_erc20, false);
  EXPECT_EQ(it->second[1]->is_erc721, false);
  EXPECT_EQ(it->second[1]->is_erc1155, false);
  EXPECT_EQ(it->second[1]->is_nft, false);
  EXPECT_EQ(it->second[1]->symbol, "ETH");
  EXPECT_EQ(it->second[1]->decimals, 18);
  EXPECT_EQ(it->second[1]->visible, true);
  EXPECT_EQ(it->second[1]->token_id, "");
  EXPECT_EQ(it->second[1]->coingecko_id, "");
  EXPECT_EQ(it->second[1]->chain_id, "0xa");
  EXPECT_EQ(it->second[1]->coin, mojom::CoinType::ETH);
}

TEST(BlockchainListParseUnitTest, ParseOffRampTokensListMap) {
  const std::string supported_tokens_list = R"({
    "tokens": [
      {
        "chain_id": "0x1",
        "coin": 60,
        "coingecko_id": "",
        "contract_address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
        "decimals": 18,
        "is_erc1155": false,
        "is_erc20": true,
        "is_erc721": false,
        "is_nft": false,
        "logo": "bat.png",
        "name": "Basic Attention Token",
        "symbol": "BAT",
        "token_id": "",
        "visible": true,
        "on_ramp_providers": ["ramp", "sardine", "transak", "stripe"],
        "off_ramp_providers": ["ramp"]
      }
    ]
  })";

  std::optional<RampTokenListMaps> supported_tokens_list_map =
      ParseRampTokenListMaps(supported_tokens_list);
  ASSERT_TRUE(supported_tokens_list_map);
  EXPECT_EQ((*supported_tokens_list_map).second.size(), 1UL);

  auto it =
      (*supported_tokens_list_map).second.find(mojom::OffRampProvider::kRamp);
  ASSERT_NE(it, (*supported_tokens_list_map).second.end());
  EXPECT_EQ(it->second[0]->contract_address,
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_EQ(it->second[0]->name, "Basic Attention Token");
  EXPECT_EQ(it->second[0]->logo, "bat.png");
  EXPECT_EQ(it->second[0]->is_erc20, true);
  EXPECT_EQ(it->second[0]->is_erc721, false);
  EXPECT_EQ(it->second[0]->is_erc1155, false);
  EXPECT_EQ(it->second[0]->is_nft, false);
  EXPECT_EQ(it->second[0]->symbol, "BAT");
  EXPECT_EQ(it->second[0]->decimals, 18);
  EXPECT_EQ(it->second[0]->visible, true);
  EXPECT_EQ(it->second[0]->token_id, "");
  EXPECT_EQ(it->second[0]->coingecko_id, "");
  EXPECT_EQ(it->second[0]->chain_id, "0x1");
  EXPECT_EQ(it->second[0]->coin, mojom::CoinType::ETH);
}

TEST(ParseOnRampCurrencyListTest, ParseOnRampCurrencyLists) {
  // Invalid JSON is not parsed
  std::optional<std::vector<mojom::OnRampCurrency>> supported_currencies_lists =
      ParseOnRampCurrencyLists(R"({)");
  ASSERT_FALSE(supported_currencies_lists);

  const std::string supported_currencies_lists_json = R"({
    "currencies": [
      {
        "currency_code": "ARS",
        "currency_name": "Argentine Peso",
        "providers": [
          "transak"
        ]
      },
      {
        "currency_code": "USD",
        "currency_name": "US Dollar",
        "providers": [
          "ramp",
          "transak",
          "stripe"
        ]
      }
    ]
  })";

  supported_currencies_lists =
      ParseOnRampCurrencyLists(supported_currencies_lists_json);
  ASSERT_TRUE(supported_currencies_lists);

  // Check size of the vector
  EXPECT_EQ(supported_currencies_lists->size(), 2UL);

  // Check values for the first currency
  EXPECT_EQ((*supported_currencies_lists)[0].currency_code, "ARS");
  EXPECT_EQ((*supported_currencies_lists)[0].currency_name, "Argentine Peso");
  EXPECT_EQ((*supported_currencies_lists)[0].providers.size(), 1UL);
  EXPECT_EQ((*supported_currencies_lists)[0].providers[0],
            mojom::OnRampProvider::kTransak);

  // Check values for the second currency
  EXPECT_EQ((*supported_currencies_lists)[1].currency_code, "USD");
  EXPECT_EQ((*supported_currencies_lists)[1].currency_name, "US Dollar");
  EXPECT_EQ((*supported_currencies_lists)[1].providers.size(), 3UL);
  EXPECT_EQ((*supported_currencies_lists)[1].providers[0],
            mojom::OnRampProvider::kRamp);
  EXPECT_EQ((*supported_currencies_lists)[1].providers[1],
            mojom::OnRampProvider::kTransak);
  EXPECT_EQ((*supported_currencies_lists)[1].providers[2],
            mojom::OnRampProvider::kStripe);
}

TEST(BlockchainListParseUnitTest, ParseCoingeckoIdsMap) {
  const std::string json = R"({
    "0x1": {
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78": "0chain",
      "0xe41d2489571d322189246dafa5ebde1f4699f498": "0x",
      "0x5a3e6a77ba2f983ec0d371ea3b475f8bc0811ad5": "0x0-ai-ai-smart-contract",
      "0xfcdb9e987f9159dab2f507007d5e3d10c510aa70": "0x1-tools-ai-multi-tool"
    }
  })";

  std::optional<CoingeckoIdsMap> coingecko_ids_map = ParseCoingeckoIdsMap(json);

  ASSERT_TRUE(coingecko_ids_map);

  EXPECT_EQ((*coingecko_ids_map)[std::pair(
                "0x1", "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78")],
            "0chain");

  EXPECT_EQ((*coingecko_ids_map)[std::pair(
                "0x1", "0xe41d2489571d322189246dafa5ebde1f4699f498")],
            "0x");

  EXPECT_EQ((*coingecko_ids_map)[std::pair(
                "0x1", "0x5a3e6a77ba2f983ec0d371ea3b475f8bc0811ad5")],
            "0x0-ai-ai-smart-contract");

  EXPECT_EQ((*coingecko_ids_map)[std::pair(
                "0x1", "0xfcdb9e987f9159dab2f507007d5e3d10c510aa70")],
            "0x1-tools-ai-multi-tool");

  EXPECT_FALSE(coingecko_ids_map->contains({"0x2", "0xdeadbeef"}));
}

TEST(BlockchainListParseUnitTest, ParseOfacAddressesList) {
  const std::string json = R"({
    "addresses": [
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78",
      "0xE41D2489571D322189246DAFA5EBDE1F4699F498",
      "0x5a3e6a77ba2f983ec0d371ea3b475f8bc0811ad5",
      "0xfcdb9e987f9159dab2f507007d5e3d10c510aa70"
    ]
  })";

  std::optional<std::vector<std::string>> ofac_addresses_list =
      ParseOfacAddressesList(json);
  ASSERT_TRUE(ofac_addresses_list);

  EXPECT_EQ((*ofac_addresses_list).size(), 4U);
  EXPECT_EQ((*ofac_addresses_list)[0],
            "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78");
  EXPECT_EQ((*ofac_addresses_list)[1],
            "0xe41d2489571d322189246dafa5ebde1f4699f498");
}

}  // namespace brave_wallet
