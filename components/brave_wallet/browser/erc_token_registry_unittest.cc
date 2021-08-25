/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/erc_token_list_parser.h"
#include "brave/components/brave_wallet/browser/erc_token_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

const char token_list_json[] = R"(
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
   },
   "0x6090A6e47849629b7245Dfa1Ca21D94cd15878Ef": {
    "name": "ENS Registrar",
    "logo": "ens.svg"
   }
  })";

}  // namespace

TEST(ERCTokenRegistryUnitTest, GetAllTokens) {
  auto* registry = ERCTokenRegistry::GetInstance();
  std::vector<mojom::ERCTokenPtr> input_erc_tokens;
  ASSERT_TRUE(ParseTokenList(token_list_json, &input_erc_tokens));
  registry->UpdateTokenList(std::move(input_erc_tokens));

  registry->GetAllTokens(
      base::BindOnce([](std::vector<mojom::ERCTokenPtr> token_list) {
        // ENS Registrar should not be parsed because it doesn't have decimals
        // nor a symbol defined
        ASSERT_EQ(token_list.size(), 3UL);

        ASSERT_EQ(token_list[0]->name, "Crypto Kitties");
        ASSERT_EQ(token_list[0]->contract_address,
                  "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
        ASSERT_FALSE(token_list[0]->is_erc20);
        ASSERT_TRUE(token_list[0]->is_erc721);
        ASSERT_EQ(token_list[0]->symbol, "CK");
        ASSERT_EQ(token_list[0]->decimals, 0);

        ASSERT_EQ(token_list[1]->name, "Basic Attention Token");
        ASSERT_EQ(token_list[1]->contract_address,
                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
        ASSERT_TRUE(token_list[1]->is_erc20);
        ASSERT_FALSE(token_list[1]->is_erc721);
        ASSERT_EQ(token_list[1]->symbol, "BAT");
        ASSERT_EQ(token_list[1]->decimals, 18);

        ASSERT_EQ(token_list[2]->name, "Uniswap");
        ASSERT_EQ(token_list[2]->contract_address,
                  "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
        ASSERT_TRUE(token_list[2]->is_erc20);
        ASSERT_FALSE(token_list[2]->is_erc721);
        ASSERT_EQ(token_list[2]->symbol, "UNI");
        ASSERT_EQ(token_list[2]->decimals, 18);
      }));
}

TEST(ERCTokenRegistryUnitTest, GetTokenByContract) {
  auto* registry = ERCTokenRegistry::GetInstance();
  std::vector<mojom::ERCTokenPtr> input_erc_tokens;
  ASSERT_TRUE(ParseTokenList(token_list_json, &input_erc_tokens));
  registry->UpdateTokenList(std::move(input_erc_tokens));
  registry->GetTokenByContract("0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
                               base::BindOnce([](mojom::ERCTokenPtr token) {
                                 ASSERT_EQ(token->symbol, "BAT");
                               }));

  registry->GetTokenByContract(
      "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindOnce([](mojom::ERCTokenPtr token) { ASSERT_FALSE(token); }));
}

TEST(ERCTokenRegistryUnitTest, GetTokenBySymbol) {
  auto* registry = ERCTokenRegistry::GetInstance();
  std::vector<mojom::ERCTokenPtr> input_erc_tokens;
  ASSERT_TRUE(ParseTokenList(token_list_json, &input_erc_tokens));
  registry->UpdateTokenList(std::move(input_erc_tokens));
  registry->GetTokenBySymbol(
      "BAT", base::BindOnce([](mojom::ERCTokenPtr token) {
        ASSERT_EQ(token->contract_address,
                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
      }));

  registry->GetTokenBySymbol(
      "BRB",
      base::BindOnce([](mojom::ERCTokenPtr token) { ASSERT_FALSE(token); }));
}

}  // namespace brave_wallet
