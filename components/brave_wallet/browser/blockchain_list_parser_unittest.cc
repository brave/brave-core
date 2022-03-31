/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

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
       "chainId": "0x3"
     }
    }
  )");
  std::map<std::string, std::vector<mojom::BlockchainTokenPtr>> token_list_map;
  ASSERT_TRUE(ParseTokenList(json, &token_list_map));
  ASSERT_EQ(token_list_map["0x1"].size(), 2UL);
  EXPECT_EQ(token_list_map["0x2"].size(), 0UL);
  ASSERT_EQ(token_list_map["0x3"].size(), 1UL);

  const auto& mainnet_token_list = token_list_map["0x1"];
  EXPECT_EQ(mainnet_token_list[0]->name, "Crypto Kitties");
  EXPECT_EQ(mainnet_token_list[0]->contract_address,
            "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
  EXPECT_FALSE(mainnet_token_list[0]->is_erc20);
  EXPECT_TRUE(mainnet_token_list[0]->is_erc721);
  EXPECT_EQ(mainnet_token_list[0]->symbol, "CK");
  EXPECT_EQ(mainnet_token_list[0]->logo, "CryptoKitties-Kitty-13733.svg");
  EXPECT_EQ(mainnet_token_list[0]->decimals, 0);
  EXPECT_TRUE(mainnet_token_list[0]->coingecko_id.empty());

  EXPECT_EQ(mainnet_token_list[1]->name, "Basic Attention Token");
  EXPECT_EQ(mainnet_token_list[1]->contract_address,
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  EXPECT_TRUE(mainnet_token_list[1]->is_erc20);
  EXPECT_FALSE(mainnet_token_list[1]->is_erc721);
  EXPECT_EQ(mainnet_token_list[1]->symbol, "BAT");
  EXPECT_EQ(mainnet_token_list[1]->logo, "bat.svg");
  EXPECT_EQ(mainnet_token_list[1]->decimals, 18);
  EXPECT_EQ(mainnet_token_list[1]->coingecko_id, "basic-attention-token");

  const auto& ropsten_token_list = token_list_map["0x3"];
  EXPECT_EQ(ropsten_token_list[0]->name, "Uniswap");
  EXPECT_EQ(ropsten_token_list[0]->contract_address,
            "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
  EXPECT_TRUE(ropsten_token_list[0]->is_erc20);
  EXPECT_FALSE(ropsten_token_list[0]->is_erc721);
  EXPECT_EQ(ropsten_token_list[0]->symbol, "UNI");
  EXPECT_EQ(ropsten_token_list[0]->logo, "uni.svg");
  EXPECT_EQ(ropsten_token_list[0]->decimals, 18);
  EXPECT_TRUE(mainnet_token_list[0]->coingecko_id.empty());

  token_list_map.clear();
  json = R"({})";
  EXPECT_TRUE(ParseTokenList(json, &token_list_map));
  EXPECT_TRUE(token_list_map.empty());
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": 3})";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {}})";
  EXPECT_TRUE(token_list_map.empty());
  EXPECT_TRUE(ParseTokenList(json, &token_list_map));
  json = "3";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
  json = "[3]";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
  json = "";
  EXPECT_FALSE(ParseTokenList(json, &token_list_map));
}

}  // namespace brave_wallet
