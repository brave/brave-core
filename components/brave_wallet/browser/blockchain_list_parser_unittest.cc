/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"

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
      "0x1": {
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
        }
      },
      "0xaa36a7": {
        "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
          "name": "Uniswap",
          "logo": "uni.svg",
          "erc20": true,
          "symbol": "UNI",
          "decimals": 18
        }
      },
      "0x65": {
        "So11111111111111111111111111111111111111112": {
          "name": "Wrapped SOL",
          "logo": "So11111111111111111111111111111111111111112.png",
          "erc20": false,
          "symbol": "SOL",
          "decimals": 9,
          "coingeckoId": "solana"
        },
        "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v": {
          "name": "USD Coin",
          "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
          "erc20": false,
          "symbol": "USDC",
          "decimals": 6,
          "coingeckoId": "usd-coin"
        },
        "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ": {
          "name": "Tesla Inc.",
          "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
          "erc20": false,
          "token2022": true,
          "symbol": "TSLA",
          "decimals": 8
        }
      },
    "polkadot_asset_hub": {
        "1000085": {
          "coingeckoId": "gavun-wud",
          "decimals": 10,
          "logo": "https://coin-images.coingecko.com/coins/images/53409/large/logo_small.png?1766718628",
          "name": "Gavun Wud",
          "symbol": "WUD"
        },
        "1337": {
          "coingeckoId": "usd-coin",
          "decimals": 6,
          "logo": "https://coin-images.coingecko.com/coins/images/6319/large/USDC.png?1769615602",
          "name": "USDC",
          "symbol": "USDC"
        },
        "50000103": {
          "coingeckoId": "my-paqman-coin",
          "decimals": 12,
          "logo": "https://coin-images.coingecko.com/coins/images/55315/large/1000001202.jpg?1745399713",
          "name": "My Paqman Coin",
          "symbol": "MPC"
        }
      }
    }
  )");

  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(json, &token_list_map));
  ASSERT_EQ(token_list_map["ethereum.0x1"].size(), 2UL);
  EXPECT_EQ(token_list_map["ethereum.0x2"].size(), 0UL);
  ASSERT_EQ(token_list_map["ethereum.0xaa36a7"].size(), 1UL);
  ASSERT_EQ(token_list_map["polkadot.polkadot_asset_hub"].size(), 3UL);

  const auto& ethereum_token_list = token_list_map["ethereum.0x1"];
  EXPECT_EQ(ethereum_token_list[0]->name, "Crypto Kitties");
  EXPECT_EQ(ethereum_token_list[0]->contract_address,
            "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
  EXPECT_FALSE(ethereum_token_list[0]->is_erc20);
  EXPECT_TRUE(ethereum_token_list[0]->is_erc721);
  EXPECT_TRUE(ethereum_token_list[0]->is_nft);
  EXPECT_EQ(ethereum_token_list[0]->symbol, "CK");
  EXPECT_EQ(ethereum_token_list[0]->logo, "CryptoKitties-Kitty-13733.svg");
  EXPECT_EQ(ethereum_token_list[0]->decimals, 0);
  EXPECT_TRUE(ethereum_token_list[0]->coingecko_id.empty());
  EXPECT_EQ(ethereum_token_list[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

  EXPECT_EQ(ethereum_token_list[1]->name, "Basic Attention Token");
  EXPECT_EQ(ethereum_token_list[1]->contract_address,
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_TRUE(ethereum_token_list[1]->is_erc20);
  EXPECT_FALSE(ethereum_token_list[1]->is_erc721);
  EXPECT_FALSE(ethereum_token_list[1]->is_erc1155);
  EXPECT_FALSE(ethereum_token_list[1]->is_nft);
  EXPECT_EQ(ethereum_token_list[1]->symbol, "BAT");
  EXPECT_EQ(ethereum_token_list[1]->logo, "bat.svg");
  EXPECT_EQ(ethereum_token_list[1]->decimals, 18);
  EXPECT_EQ(ethereum_token_list[1]->coingecko_id, "basic-attention-token");
  EXPECT_EQ(ethereum_token_list[1]->spl_token_program,
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

  ASSERT_EQ(token_list_map["solana.0x65"].size(), 3UL);
  auto wrapped_sol = mojom::BlockchainToken::New(
      "So11111111111111111111111111111111111111112", "Wrapped SOL",
      "So11111111111111111111111111111111111111112.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "SOL", 9, true, "",
      "solana", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
  auto usdc = mojom::BlockchainToken::New(
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", "USD Coin",
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "USDC", 6, true, "",
      "usd-coin", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
  auto tsla = mojom::BlockchainToken::New(
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "Tesla Inc.",
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken2022, false, false, "TSLA", 8, true,
      "", "", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
  std::vector<mojom::BlockchainTokenPtr> solana_token_list;
  solana_token_list.push_back(std::move(tsla));
  solana_token_list.push_back(std::move(usdc));
  solana_token_list.push_back(std::move(wrapped_sol));
  EXPECT_EQ(token_list_map["solana.0x65"], solana_token_list);

  auto wud = mojom::BlockchainToken::New(
      "1000085", "Gavun Wud",
      "https://coin-images.coingecko.com/coins/images/53409/large/"
      "logo_small.png?1766718628",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, false,
      false, "WUD", 10, true, "", "gavun-wud", mojom::kPolkadotMainnetAssetHub,
      mojom::CoinType::DOT, mojom::ZCashTokenType::kNone);
  auto dot_usdc = mojom::BlockchainToken::New(
      "1337", "USDC",
      "https://coin-images.coingecko.com/coins/images/6319/large/"
      "USDC.png?1769615602",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, false,
      false, "USDC", 6, true, "", "usd-coin", mojom::kPolkadotMainnetAssetHub,
      mojom::CoinType::DOT, mojom::ZCashTokenType::kNone);
  auto mpc = mojom::BlockchainToken::New(
      "50000103", "My Paqman Coin",
      "https://coin-images.coingecko.com/coins/images/55315/large/"
      "1000001202.jpg?1745399713",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, false,
      false, "MPC", 12, true, "", "my-paqman-coin",
      mojom::kPolkadotMainnetAssetHub, mojom::CoinType::DOT,
      mojom::ZCashTokenType::kNone);
  std::vector<mojom::BlockchainTokenPtr> polkadot_token_list;
  polkadot_token_list.push_back(std::move(wud));
  polkadot_token_list.push_back(std::move(dot_usdc));
  polkadot_token_list.push_back(std::move(mpc));
  EXPECT_EQ(token_list_map["polkadot.polkadot_asset_hub"], polkadot_token_list);

  token_list_map.clear();
  json = R"({})";
  EXPECT_TRUE(ParseTokenList(json, &token_list_map));
  EXPECT_TRUE(token_list_map.empty());
  json = R"({"0x1": { "0x0D8775F648430679A709E98d2b0Cb6250d2887EF": 3}})";
  EXPECT_TRUE(ParseTokenList(json, &token_list_map));
  EXPECT_TRUE(token_list_map.empty());
  json = R"({"0x1": {}})";
  EXPECT_TRUE(ParseTokenList(json, &token_list_map));
  EXPECT_TRUE(token_list_map.empty());
  json = "3";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
  json = "[3]";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
  json = "";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
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

TEST(BlockchainListParseUnitTest, ParseRestrictedAddressesList) {
  const std::string json = R"({
    "addresses": [
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78",
      "0xE41D2489571D322189246DAFA5EBDE1F4699F498",
      "0x5a3e6a77ba2f983ec0d371ea3b475f8bc0811ad5",
      "0xfcdb9e987f9159dab2f507007d5e3d10c510aa70"
    ]
  })";

  std::optional<std::vector<std::string>> restricted_addresses_list =
      ParseRestrictedAddressesList(json);
  ASSERT_TRUE(restricted_addresses_list);

  EXPECT_EQ((*restricted_addresses_list).size(), 4U);
  EXPECT_EQ((*restricted_addresses_list)[0],
            "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78");
  EXPECT_EQ((*restricted_addresses_list)[1],
            "0xe41d2489571d322189246dafa5ebde1f4699f498");
}

TEST(BlockchainListParseUnitTest, ParsePolkadotRpcUrls) {
  const std::string json = R"({
    "polkadot_acala": ["https://acala-rpc-0.aca-api.network"],
    "polkadot_bifrost": ["https://hk.p.bifrost-rpc.liebi.com"],
    "polkadot_hydration": ["https://hydration-rpc.n.dwellir.com"],
    "polkadot_neuroweb": ["https://parachain-rpc.origin-trail.network"],
    "polkadot_jamton": ["https://rpc.jamton.network"],
    "kusama_asset_hub": [
      "https://rpc-asset-hub-kusama.luckyfriday.io",
      "https://kusama-asset-hub-rpc.polkadot.io"
    ]
  })";

  auto rpc_urls = ParsePolkadotRpcUrls(json);
  ASSERT_TRUE(rpc_urls);

  EXPECT_EQ(rpc_urls->size(), 6U);
  EXPECT_THAT(rpc_urls->at(mojom::kPolkadotAcala),
              ElementsAreArray({GURL("https://acala-rpc-0.aca-api.network")}));
  EXPECT_THAT(rpc_urls->at(mojom::kPolkadotBifrost),
              ElementsAreArray({GURL("https://hk.p.bifrost-rpc.liebi.com")}));
  EXPECT_THAT(rpc_urls->at(mojom::kPolkadotHydration),
              ElementsAreArray({GURL("https://hydration-rpc.n.dwellir.com")}));
  EXPECT_THAT(
      rpc_urls->at(mojom::kPolkadotNeuroWeb),
      ElementsAreArray({GURL("https://parachain-rpc.origin-trail.network")}));
  EXPECT_THAT(rpc_urls->at(mojom::kPolkadotJamton),
              ElementsAreArray({GURL("https://rpc.jamton.network")}));
  EXPECT_THAT(
      rpc_urls->at(mojom::kPolkadotKusamaHub),
      ElementsAreArray({GURL("https://rpc-asset-hub-kusama.luckyfriday.io"),
                        GURL("https://kusama-asset-hub-rpc.polkadot.io")}));

  EXPECT_FALSE(rpc_urls->contains(mojom::kPolkadotMainnet));

  EXPECT_TRUE(ParsePolkadotRpcUrls("{}")->empty());

  EXPECT_FALSE(ParsePolkadotRpcUrls(""));
  EXPECT_FALSE(ParsePolkadotRpcUrls("[]"));
  EXPECT_FALSE(ParsePolkadotRpcUrls(R"({"polkadot_acala": "https://a.com"})"));
  EXPECT_FALSE(ParsePolkadotRpcUrls(R"({"polkadot_acala": [42]})"));
  EXPECT_FALSE(ParsePolkadotRpcUrls(R"({"polkadot_acala": ["not a url"]})"));
}

}  // namespace brave_wallet
