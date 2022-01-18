/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "content/public/test/browser_task_environment.h"
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
     "decimals": 18,
     "chainId": "0x3"
   },
   "0x6090A6e47849629b7245Dfa1Ca21D94cd15878Ef": {
    "name": "ENS Registrar",
    "logo": "ens.svg"
   }
  })";

}  // namespace

TEST(BlockchainRegistryUnitTest, GetAllTokens) {
  content::BrowserTaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(token_list_json, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));

  // Loop twice to make sure getting the same list twice works
  // For example, make sure nothing is std::move'd
  for (size_t i = 0; i < 2; ++i) {
    base::RunLoop run_loop;
    registry->GetAllTokens(
        mojom::kMainnetChainId,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
              // ENS Registrar should not be parsed because it doesn't
              // have decimals nor a symbol defined
              ASSERT_EQ(token_list.size(), 2UL);

              EXPECT_EQ(token_list[0]->name, "Crypto Kitties");
              EXPECT_EQ(token_list[0]->contract_address,
                        "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
              EXPECT_FALSE(token_list[0]->is_erc20);
              EXPECT_TRUE(token_list[0]->is_erc721);
              EXPECT_EQ(token_list[0]->symbol, "CK");
              EXPECT_EQ(token_list[0]->decimals, 0);

              EXPECT_EQ(token_list[1]->name, "Basic Attention Token");
              EXPECT_EQ(token_list[1]->contract_address,
                        "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
              EXPECT_TRUE(token_list[1]->is_erc20);
              EXPECT_FALSE(token_list[1]->is_erc721);
              EXPECT_EQ(token_list[1]->symbol, "BAT");
              EXPECT_EQ(token_list[1]->decimals, 18);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetAllTokens(
      mojom::kRopstenChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            ASSERT_EQ(token_list.size(), 1UL);
            EXPECT_EQ(token_list[0]->name, "Uniswap");
            EXPECT_EQ(token_list[0]->contract_address,
                      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
            EXPECT_TRUE(token_list[0]->is_erc20);
            EXPECT_FALSE(token_list[0]->is_erc721);
            EXPECT_EQ(token_list[0]->symbol, "UNI");
            EXPECT_EQ(token_list[0]->decimals, 18);
            run_loop2.Quit();
          }));
  run_loop2.Run();

  // chainId which has no tokens
  base::RunLoop run_loop3;
  registry->GetAllTokens(
      mojom::kRinkebyChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            // ENS Registrar should not be parsed because it doesn't have
            // decimals nor a symbol defined
            EXPECT_EQ(token_list.size(), 0UL);
            run_loop3.Quit();
          }));
  run_loop3.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenByContract) {
  content::BrowserTaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(token_list_json, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenByContract(
      mojom::kMainnetChainId, "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "BAT");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenByContract(
      mojom::kRopstenChainId, "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "UNI");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // tokens for chanId exist but contract doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenByContract(
      mojom::kMainnetChainId, "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenByContract(
      mojom::kRinkebyChainId, "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenBySymbol) {
  content::BrowserTaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(token_list_json, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, "BAT",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenBySymbol(
      mojom::kRopstenChainId, "UNI",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // chainId has tokens but token doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenBySymbol(
      mojom::kRinkebyChainId, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();
}

}  // namespace brave_wallet
