/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/erc_token_list_parser.h"
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
       "decimals": 18
     },
     "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
       "name": "Uniswap",
       "logo": "uni.svg",
       "erc20": true,
       "symbol": "UNI",
       "decimals": 18
     }
    }
  )");
  std::vector<mojom::ERCTokenPtr> token_list;
  ASSERT_TRUE(ParseTokenList(json, &token_list));
  ASSERT_EQ(token_list.size(), 3UL);

  ASSERT_EQ(token_list[0]->name, "Crypto Kitties");
  ASSERT_EQ(token_list[0]->contract_address,
            "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
  ASSERT_FALSE(token_list[0]->is_erc20);
  ASSERT_TRUE(token_list[0]->is_erc721);
  ASSERT_EQ(token_list[0]->symbol, "CK");
  ASSERT_EQ(token_list[0]->logo, "CryptoKitties-Kitty-13733.svg");
  ASSERT_EQ(token_list[0]->decimals, 0);

  ASSERT_EQ(token_list[1]->name, "Basic Attention Token");
  ASSERT_EQ(token_list[1]->contract_address,
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  ASSERT_TRUE(token_list[1]->is_erc20);
  ASSERT_FALSE(token_list[1]->is_erc721);
  ASSERT_EQ(token_list[1]->symbol, "BAT");
  ASSERT_EQ(token_list[1]->logo, "bat.svg");
  ASSERT_EQ(token_list[1]->decimals, 18);

  ASSERT_EQ(token_list[2]->name, "Uniswap");
  ASSERT_EQ(token_list[2]->contract_address,
            "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
  ASSERT_TRUE(token_list[2]->is_erc20);
  ASSERT_FALSE(token_list[2]->is_erc721);
  ASSERT_EQ(token_list[2]->symbol, "UNI");
  ASSERT_EQ(token_list[2]->logo, "uni.svg");
  ASSERT_EQ(token_list[2]->decimals, 18);

  token_list.clear();
  json = R"({})";
  ASSERT_TRUE(ParseTokenList(json, &token_list));
  ASSERT_TRUE(token_list.empty());
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": 3})";
  ASSERT_FALSE(ParseTokenList(json, &token_list));
  json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {}})";
  ASSERT_EQ(token_list.size(), 0UL);
  ASSERT_TRUE(ParseTokenList(json, &token_list));
  json = "3";
  ASSERT_FALSE(ParseTokenList(json, &token_list));
  json = "[3]";
  ASSERT_FALSE(ParseTokenList(json, &token_list));
  json = "";
  ASSERT_FALSE(ParseTokenList(json, &token_list));
}

}  // namespace brave_wallet
