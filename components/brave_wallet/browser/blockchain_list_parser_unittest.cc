/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAreArray;

namespace brave_wallet {

TEST(ParseTokenListUnitTest, ParseTokenList) {
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
       "chainId": "0x5"
     }
    }
  )");

  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(json, &token_list_map, mojom::CoinType::ETH));
  ASSERT_EQ(token_list_map["ethereum.0x1"].size(), 2UL);
  EXPECT_EQ(token_list_map["ethereum.0x2"].size(), 0UL);
  ASSERT_EQ(token_list_map["ethereum.0x5"].size(), 1UL);

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

  const auto& goerli_token_list = token_list_map["ethereum.0x5"];
  EXPECT_EQ(goerli_token_list[0]->name, "Uniswap");
  EXPECT_EQ(goerli_token_list[0]->contract_address,
            "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
  EXPECT_TRUE(goerli_token_list[0]->is_erc20);
  EXPECT_FALSE(goerli_token_list[0]->is_erc721);
  EXPECT_FALSE(goerli_token_list[0]->is_erc1155);
  EXPECT_FALSE(goerli_token_list[0]->is_nft);
  EXPECT_EQ(goerli_token_list[0]->symbol, "UNI");
  EXPECT_EQ(goerli_token_list[0]->logo, "uni.svg");
  EXPECT_EQ(goerli_token_list[0]->decimals, 18);
  EXPECT_TRUE(mainnet_token_list[0]->coingecko_id.empty());

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
      false, "SOL", 9, true, "", "solana", "0x65", mojom::CoinType::SOL);
  auto usdc = mojom::BlockchainToken::New(
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", "USD Coin",
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png", false, false, false,
      false, "USDC", 6, true, "", "usd-coin", "0x65", mojom::CoinType::SOL);
  auto tsla = mojom::BlockchainToken::New(
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "Tesla Inc.",
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png", false, false, false,
      false, "TSLA", 8, true, "", "", "0x65", mojom::CoinType::SOL);
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

TEST(ParseChainListUnitTest, ParseChainList) {
  const std::string chain_list = R"(
  [
    {
      "name": "Ethereum Mainnet",
      "chain": "ETH",
      "icon": "ethereum",
      "rpc": [
        "https://mainnet.infura.io/v3/${INFURA_API_KEY}",
        "wss://mainnet.infura.io/ws/v3/${INFURA_API_KEY}",
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
                        GURL("wss://mainnet.infura.io/ws/v3/${INFURA_API_KEY}"),
                        GURL("https://api.mycryptoapi.com/eth"),
                        GURL("https://cloudflare-eth.com")}));
  EXPECT_EQ(2, chain1->active_rpc_endpoint_index);
  EXPECT_THAT(chain1->block_explorer_urls,
              ElementsAreArray({"https://etherscan.io"}));
  EXPECT_EQ("Ether", chain1->symbol_name);
  EXPECT_EQ("ETH", chain1->symbol);
  EXPECT_EQ(18, chain1->decimals);
  EXPECT_EQ(0u, chain1->icon_urls.size());
  EXPECT_EQ(chain1->coin, mojom::CoinType::ETH);
  EXPECT_FALSE(chain1->is_eip1559);

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
  EXPECT_FALSE(chain2->is_eip1559);
}

TEST(ParseDappListsUnitTest, ParseDappLists) {
  const std::string dapp_list = R"({
    "ethereum": {
      "success": true,
      "chain": "ethereum",
      "category": null,
      "range": "30d",
      "top": 10,
      "results": [
        {
          "dappId": 7000,
          "name": "Uniswap V3",
          "description": "A protocol for trading and automated liquidity.",
          "fullDescription": "",
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
          "socialLinks": [
            {
              "title": "blog",
              "url": "https://uniswap.org/blog/",
              "type": "blog"
            },
            {
              "title": "discord",
              "url": "https://discord.com/invite/FCfyBSbCU5",
              "type": "discord"
            },
            {
              "title": "github",
              "url": "https://github.com/Uniswap",
              "type": "github"
            },
            {
              "title": "reddit",
              "url": "https://www.reddit.com/r/UniSwap/",
              "type": "reddit"
            },
            {
              "title": "twitter",
              "url": "https://twitter.com/Uniswap",
              "type": "twitter"
            }
          ],
          "metrics": {
            "transactions": 2348167,
            "uaw": 387445,
            "volume": 65982226285.39,
            "balance": 1904817795.53
          }
        }
      ]
    },
    "solana": {
      "success": true,
      "chain": "solana",
      "category": null,
      "range": "30d",
      "top": 10,
      "results": [
        {
          "dappId": 20419,
          "name": "GameTrade Market",
          "description": "Discover, buy, sell and trade in-game NFTs",
          "fullDescription": "",
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
          "socialLinks": [
            {
              "title": "discord",
              "url": "https://discord.gg/gametrade",
              "type": "discord"
            },
            {
              "title": "medium",
              "url": "https://gametrademarket.medium.com/",
              "type": "medium"
            },
            {
              "title": "twitter",
              "url": "https://twitter.com/GameTradeMarket",
              "type": "twitter"
            },
            {
              "title": "youtube",
              "url": "https://www.youtube.com/channel/UCAoMHO4zQaiT-vxWOVk8IjA/videos",
              "type": "youtube"
            }
          ],
          "metrics": {
            "transactions": 401926,
            "uaw": 354495,
            "volume": 8949.83,
            "balance": 3.81
          }
        }
      ]
    }
  })";

  // Parse the dapp list
  DappListMap dapp_list_map;
  EXPECT_TRUE(ParseDappLists(dapp_list, &dapp_list_map));

  // There should be two lists, one for Ethereum and one for Solana
  ASSERT_EQ(2u, dapp_list_map.size());

  // There should be one dapp in the Ethereum list
  auto it = dapp_list_map.find(
      GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId));
  EXPECT_TRUE(it != dapp_list_map.end());
  const auto& eth_dapp_list = it->second;
  EXPECT_EQ(eth_dapp_list.size(), 1u);
  const auto& eth_dapp = eth_dapp_list[0];
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
  EXPECT_EQ(eth_dapp->transactions, 2348167u);
  EXPECT_EQ(eth_dapp->uaw, 387445u);
  EXPECT_DOUBLE_EQ(eth_dapp->volume, 65982226285.39);
  EXPECT_DOUBLE_EQ(eth_dapp->balance, 1904817795.53);

  // There should be one dapp in the Solana list
  auto it_s = dapp_list_map.find(
      GetTokenListKey(mojom::CoinType::SOL, mojom::kSolanaMainnet));
  EXPECT_TRUE(it_s != dapp_list_map.end());
  const auto& sol_dapp_list = it_s->second;
  EXPECT_EQ(sol_dapp_list.size(), 1u);
  const auto& sol_dapp = sol_dapp_list[0];
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
  EXPECT_EQ(sol_dapp->transactions, 401926u);
  EXPECT_EQ(sol_dapp->uaw, 354495u);
  EXPECT_DOUBLE_EQ(sol_dapp->volume, 8949.83);
  EXPECT_DOUBLE_EQ(sol_dapp->balance, 3.81);
}

}  // namespace brave_wallet
