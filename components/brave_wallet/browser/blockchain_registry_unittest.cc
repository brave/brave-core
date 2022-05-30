/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
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

const char solana_token_list_json[] = R"(
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
  })";

mojom::BlockchainTokenPtr wrapped_sol = mojom::BlockchainToken::New(
    "So11111111111111111111111111111111111111112",
    "Wrapped SOL",
    "So11111111111111111111111111111111111111112.png",
    false,
    false,
    "SOL",
    9,
    true,
    "",
    "solana",
    "0x65",
    mojom::CoinType::SOL);
mojom::BlockchainTokenPtr usdc = mojom::BlockchainToken::New(
    "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
    "USD Coin",
    "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
    false,
    false,
    "USDC",
    6,
    true,
    "",
    "usd-coin",
    "0x65",
    mojom::CoinType::SOL);
mojom::BlockchainTokenPtr tsla = mojom::BlockchainToken::New(
    "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ",
    "Tesla Inc.",
    "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
    false,
    false,
    "TSLA",
    8,
    true,
    "",
    "",
    "0x65",
    mojom::CoinType::SOL);

}  // namespace

TEST(BlockchainRegistryUnitTest, GetAllTokens) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  ASSERT_TRUE(ParseTokenList(solana_token_list_json, &token_list_map,
                             mojom::CoinType::SOL));
  registry->UpdateTokenList(std::move(token_list_map));

  // Loop twice to make sure getting the same list twice works
  // For example, make sure nothing is std::move'd
  for (size_t i = 0; i < 2; ++i) {
    base::RunLoop run_loop;
    registry->GetAllTokens(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
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
      mojom::kRopstenChainId, mojom::CoinType::ETH,
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
      mojom::kRinkebyChainId, mojom::CoinType::ETH,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            // ENS Registrar should not be parsed because it doesn't have
            // decimals nor a symbol defined
            EXPECT_EQ(token_list.size(), 0UL);
            run_loop3.Quit();
          }));
  run_loop3.Run();

  // Get Solana tokens
  base::RunLoop run_loop4;
  registry->GetAllTokens(
      mojom::kSolanaMainnet, mojom::CoinType::SOL,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            ASSERT_EQ(token_list.size(), 3UL);
            EXPECT_EQ(token_list[0], tsla);
            EXPECT_EQ(token_list[1], usdc);
            EXPECT_EQ(token_list[2], wrapped_sol);
            run_loop4.Quit();
          }));
  run_loop4.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenByAddress) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  ASSERT_TRUE(ParseTokenList(solana_token_list_json, &token_list_map,
                             mojom::CoinType::SOL));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenByAddress(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "BAT");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenByAddress(
      mojom::kRopstenChainId, mojom::CoinType::ETH,
      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "UNI");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // tokens for chanId exist but address doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenByAddress(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenByAddress(
      mojom::kRinkebyChainId, mojom::CoinType::ETH,
      "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();

  // Get Solana token
  base::RunLoop run_loop5;
  registry->GetTokenByAddress(
      mojom::kSolanaMainnet, mojom::CoinType::SOL,
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token, usdc);
        run_loop5.Quit();
      }));
  run_loop5.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenBySymbol) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  ASSERT_TRUE(ParseTokenList(solana_token_list_json, &token_list_map,
                             mojom::CoinType::SOL));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "BAT",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenBySymbol(
      mojom::kRopstenChainId, mojom::CoinType::ETH, "UNI",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // chainId has tokens but token doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenBySymbol(
      mojom::kRinkebyChainId, mojom::CoinType::ETH, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();

  // Get Solana token
  base::RunLoop run_loop5;
  registry->GetTokenBySymbol(
      mojom::kSolanaMainnet, mojom::CoinType::SOL, "USDC",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token, usdc);
        run_loop5.Quit();
      }));
  run_loop5.Run();
}

TEST(BlockchainRegistryUnitTest, GetBuyTokens) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  // Get Wyre buy tokens
  base::RunLoop run_loop1;
  registry->GetBuyTokens(
      mojom::OnRampProvider::kWyre, mojom::kMainnetChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            EXPECT_NE(token_list.size(), 0UL);
            EXPECT_EQ(token_list[0]->name, "Basic Attention Token");
            EXPECT_EQ(token_list[1]->name, "Ethereum");

            run_loop1.Quit();
          }));
  run_loop1.Run();

  base::RunLoop run_loop2;
  registry->GetBuyTokens(
      mojom::OnRampProvider::kWyre, mojom::kPolygonMainnetChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            EXPECT_EQ(token_list.size(), 2UL);
            run_loop2.Quit();
          }));
  run_loop2.Run();

  // Get Ramp buy tokens
  base::RunLoop run_loop3;
  registry->GetBuyTokens(
      mojom::OnRampProvider::kRamp, mojom::kMainnetChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            EXPECT_NE(token_list.size(), 0UL);
            EXPECT_EQ(token_list[0]->name, "Ethereum");
            run_loop3.Quit();
          }));
  run_loop3.Run();

  base::RunLoop run_loop4;
  registry->GetBuyTokens(
      mojom::OnRampProvider::kRamp, mojom::kPolygonMainnetChainId,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            EXPECT_NE(token_list.size(), 0UL);
            EXPECT_EQ(token_list[0]->name, "Polygon");
            run_loop4.Quit();
          }));
  run_loop4.Run();
}

TEST(BlockchainRegistryUnitTest, GetBuyUrlWyre) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  base::RunLoop run_loop;
  registry->GetBuyUrl(
      mojom::OnRampProvider::kWyre, mojom::kMainnetChainId, "0xdeadbeef",
      "USDC", "99.99",
      base::BindLambdaForTesting([&](const std::string& url,
                                     const absl::optional<std::string>& error) {
        EXPECT_EQ(url,
                  "https://pay.sendwyre.com/"
                  "?dest=ethereum:0xdeadbeef&destCurrency=USDC&amount=99.99&"
                  "accountId=AC_MGNVBGHPA9T&paymentMethod=debit-card");
        EXPECT_FALSE(error);

        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST(BlockchainRegistryUnitTest, GetBuyUrlRamp) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  base::RunLoop run_loop;
  registry->GetBuyUrl(
      mojom::OnRampProvider::kRamp, mojom::kMainnetChainId, "0xdeadbeef",
      "USDC",
      "55000000",  // 55 USDC
      base::BindLambdaForTesting([&](const std::string& url,
                                     const absl::optional<std::string>& error) {
        EXPECT_EQ(url,
                  "https://buy.ramp.network/"
                  "?userAddress=0xdeadbeef&swapAsset=USDC&fiatValue=55000000"
                  "&fiatCurrency=USD&hostApiKey="
                  "8yxja8782as5essk2myz3bmh4az6gpq4nte9n2gf");
        EXPECT_FALSE(error);

        run_loop.Quit();
      }));
  run_loop.Run();
}

}  // namespace brave_wallet
