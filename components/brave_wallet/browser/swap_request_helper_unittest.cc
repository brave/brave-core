/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

constexpr char kJupiterQuoteTemplate[] = R"(
    {
      "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "100000000",
      "outputMint": "%s",
      "outAmount": "10886298",
      "otherAmountThreshold": "10885210",
      "swapMode": "ExactIn",
      "slippageBps": "1",
      "platformFee": {
        "amount": "93326",
        "feeBps": "85"
      },
      "priceImpactPct": "0",
      "routePlan": [
        {
          "swapInfo": {
            "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
            "label": "Lifinity V2",
            "inputMint": "So11111111111111111111111111111111111111112",
            "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "inAmount": "100000000",
            "outAmount": "10964919",
            "feeAmount": "20000",
            "feeMint": "So11111111111111111111111111111111111111112"
          },
          "percent": "100"
        },
        {
          "swapInfo": {
            "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
            "label": "Mercurial",
            "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "inAmount": "10964919",
            "outAmount": "10979624",
            "feeAmount": "1098",
            "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
          },
          "percent": "100"
        }
      ]
    })";

}  // namespace

TEST(SwapRequestHelperUnitTest, EncodeJupiterTransactionParams) {
  std::string json =
      absl::StrFormat(kJupiterQuoteTemplate,
                      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");  // USDC
  mojom::JupiterQuotePtr swap_quote =
      jupiter::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  mojom::JupiterTransactionParams params;
  params.quote = swap_quote.Clone();
  params.user_public_key = "mockPubKey";
  auto encoded_params = jupiter::EncodeTransactionParams(params);

  std::string expected_params(R"(
    {
      "feeAccount": "7mLVS86WouwN6FCv4VwgFxX4z1GtzFk1GstjQcukrAtX",
      "quoteResponse": {
        "inputMint": "So11111111111111111111111111111111111111112",
        "inAmount": "100000000",
        "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
        "outAmount": "10886298",
        "otherAmountThreshold": "10885210",
        "swapMode": "ExactIn",
        "slippageBps": 1,
        "platformFee": {
          "amount": "93326",
          "feeBps": 85
        },
        "priceImpactPct": "0",
        "routePlan": [
          {
            "swapInfo": {
              "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
              "label": "Lifinity V2",
              "inputMint": "So11111111111111111111111111111111111111112",
              "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "inAmount": "100000000",
              "outAmount": "10964919",
              "feeAmount": "20000",
              "feeMint": "So11111111111111111111111111111111111111112"
            },
            "percent": 100
          },
          {
            "swapInfo": {
              "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
              "label": "Mercurial",
              "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "inAmount": "10964919",
              "outAmount": "10979624",
              "feeAmount": "1098",
              "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
            },
            "percent": 100
          }
        ]
      },
      "userPublicKey": "mockPubKey",
      "dynamicComputeUnitLimit": true,
      "prioritizationFeeLamports": {
        "priorityLevelWithMaxLamports": {
          "priorityLevel": "high",
          "maxLamports": 4000000,
          "global": false
        }
      }
    })");

  // OK: Jupiter transaction params with feeAccount
  auto expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(ParseJson(*encoded_params), expected_params_value);

  // OK: Jupiter transaction params WITHOUT feeAccount
  params.quote->platform_fee = nullptr;
  encoded_params = jupiter::EncodeTransactionParams(params);
  expected_params = R"(
    {
      "quoteResponse": {
        "inputMint": "So11111111111111111111111111111111111111112",
        "inAmount": "100000000",
        "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
        "outAmount": "10886298",
        "otherAmountThreshold": "10885210",
        "swapMode": "ExactIn",
        "slippageBps": 1,
        "platformFee": null,
        "priceImpactPct": "0",
        "routePlan": [
          {
            "swapInfo": {
              "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
              "label": "Lifinity V2",
              "inputMint": "So11111111111111111111111111111111111111112",
              "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "inAmount": "100000000",
              "outAmount": "10964919",
              "feeAmount": "20000",
              "feeMint": "So11111111111111111111111111111111111111112"
            },
            "percent": 100
          },
          {
            "swapInfo": {
              "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
              "label": "Mercurial",
              "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "inAmount": "10964919",
              "outAmount": "10979624",
              "feeAmount": "1098",
              "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
            },
            "percent": 100
          }
        ]
      },
      "userPublicKey": "mockPubKey",
      "dynamicComputeUnitLimit": true,
      "prioritizationFeeLamports": {
        "priorityLevelWithMaxLamports": {
          "priorityLevel": "high",
          "maxLamports": 4000000,
          "global": false
        }
      }
    })";
  expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(ParseJson(*encoded_params), expected_params_value);

  // KO: invalid output mint
  params.quote->output_mint = "invalid output mint";
  encoded_params = jupiter::EncodeTransactionParams(params);
  ASSERT_EQ(encoded_params, std::nullopt);
  encoded_params = jupiter::EncodeTransactionParams(params);
  ASSERT_EQ(encoded_params, std::nullopt);
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsLiFi) {
  // Test Gate3 LiFi encoding for EVM cross-chain swap
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->from_chain_id = mojom::kMainnetChainId;
  params->from_token = "0xdac17f958d2ee523a2206206994597c13d831ec7";
  params->from_amount = "1000000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->to_chain_id = mojom::kArbitrumMainnetChainId;
  params->to_token = "0xaf88d065e77c8cc2239327c5edb3a432268e5831";
  params->slippage_percentage = "3";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kLiFi;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_TRUE(encoded_params);

  auto parsed = ParseJson(*encoded_params);
  ASSERT_TRUE(parsed.is_dict());
  EXPECT_EQ(*parsed.GetDict().FindString("provider"), "LIFI");
  EXPECT_EQ(*parsed.GetDict().FindString("sourceTokenAddress"),
            "0xdac17f958d2ee523a2206206994597c13d831ec7");
  EXPECT_EQ(*parsed.GetDict().FindString("amount"), "1000000");
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsJupiter) {
  // Test Gate3 Jupiter encoding for Solana same-chain swap
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived,
                    "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->from_chain_id = mojom::kSolanaMainnet;
  params->from_token = "";  // native SOL
  params->from_amount = "1000000000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived,
                    "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->to_chain_id = mojom::kSolanaMainnet;
  params->to_token = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";  // USDC
  params->slippage_percentage = "0.5";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kJupiter;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "sourceCoin": "SOL",
      "sourceChainId": "0x65",
      "sourceTokenAddress": "",
      "refundTo": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
      "destinationCoin": "SOL",
      "destinationChainId": "0x65",
      "destinationTokenAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      "recipient": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
      "swapType": "EXACT_INPUT",
      "amount": "1000000000",
      "slippagePercentage": "0.5",
      "provider": "JUPITER",
      "routePriority": "CHEAPEST"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsErc20ToSpl) {
  // Test ERC20 (USDC on Ethereum) to SPL (USDC on Solana) cross-chain swap
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->from_chain_id = mojom::kMainnetChainId;
  params->from_token =
      "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48";  // USDC on Ethereum
  params->from_amount = "1000000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived,
                    "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->to_chain_id = mojom::kSolanaMainnet;
  params->to_token =
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";  // USDC on Solana
  params->slippage_percentage = "0.5";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "sourceCoin": "ETH",
      "sourceChainId": "0x1",
      "sourceTokenAddress": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
      "refundTo": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
      "destinationCoin": "SOL",
      "destinationChainId": "0x65",
      "destinationTokenAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      "recipient": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
      "swapType": "EXACT_INPUT",
      "amount": "1000000",
      "slippagePercentage": "0.5",
      "provider": "NEAR_INTENTS",
      "routePriority": "CHEAPEST"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsNativeSolToNativeBtc) {
  // Test native SOL to native BTC cross-chain swap
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived,
                    "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->from_chain_id = mojom::kSolanaMainnet;
  params->from_token = "";             // Native SOL
  params->from_amount = "1000000000";  // 1 SOL
  // BTC uses index-based account IDs, create directly
  params->to_account_id = mojom::AccountId::New(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived,
      "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq", 0, "btc_account_key");
  params->to_chain_id = mojom::kBitcoinMainnet;
  params->to_token = "";  // Native BTC
  params->slippage_percentage = "1.0";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "sourceCoin": "SOL",
      "sourceChainId": "0x65",
      "sourceTokenAddress": "",
      "refundTo": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
      "destinationCoin": "BTC",
      "destinationChainId": "bitcoin_mainnet",
      "destinationTokenAddress": "",
      "recipient": "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq",
      "swapType": "EXACT_INPUT",
      "amount": "1000000000",
      "slippagePercentage": "1.0",
      "provider": "NEAR_INTENTS",
      "routePriority": "CHEAPEST"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsNativeBtcToNativeEth) {
  // Test native BTC to native ETH cross-chain swap
  auto params = mojom::SwapQuoteParams::New();
  // BTC uses index-based account IDs, create directly
  params->from_account_id = mojom::AccountId::New(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived,
      "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq", 0, "btc_account_key");
  params->from_chain_id = mojom::kBitcoinMainnet;
  params->from_token = "";  // Native BTC
  params->from_amount = "100000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->to_chain_id = mojom::kMainnetChainId;
  params->to_token = "";  // Native ETH
  params->slippage_percentage = "0.5";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "sourceCoin": "BTC",
      "sourceChainId": "bitcoin_mainnet",
      "sourceTokenAddress": "",
      "refundTo": "bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq",
      "destinationCoin": "ETH",
      "destinationChainId": "0x1",
      "destinationTokenAddress": "",
      "recipient": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
      "swapType": "EXACT_INPUT",
      "amount": "100000",
      "slippagePercentage": "0.5",
      "provider": "NEAR_INTENTS",
      "routePriority": "CHEAPEST"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsErc20ToErc20) {
  // Test ERC20 (USDC) to ERC20 (USDT) same-chain swap on Ethereum
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->from_chain_id = mojom::kMainnetChainId;
  params->from_token =
      "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48";  // USDC on Ethereum
  params->from_amount = "1000000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->to_chain_id = mojom::kMainnetChainId;
  params->to_token =
      "0xdAC17F958D2ee523a2206206994597C13D831ec7";  // USDT on Ethereum
  params->slippage_percentage = "0.5";
  params->route_priority = mojom::RoutePriority::kCheapest;
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "sourceCoin": "ETH",
      "sourceChainId": "0x1",
      "sourceTokenAddress": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
      "refundTo": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
      "destinationCoin": "ETH",
      "destinationChainId": "0x1",
      "destinationTokenAddress": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
      "recipient": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
      "swapType": "EXACT_INPUT",
      "amount": "1000000",
      "slippagePercentage": "0.5",
      "provider": "NEAR_INTENTS",
      "routePriority": "CHEAPEST"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3QuoteParamsAllCoinTypes) {
  // Test that all supported coin types are correctly encoded
  struct CoinTypeTestCase {
    mojom::CoinType coin;
    mojom::KeyringId keyring;
    std::string expected_coin_string;
  };

  std::vector<CoinTypeTestCase> test_cases = {
      {mojom::CoinType::ETH, mojom::KeyringId::kDefault, "ETH"},
      {mojom::CoinType::SOL, mojom::KeyringId::kSolana, "SOL"},
      {mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84, "BTC"},
      {mojom::CoinType::FIL, mojom::KeyringId::kFilecoin, "FIL"},
      {mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet, "ZEC"},
      {mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet, "ADA"},
      {mojom::CoinType::DOT, mojom::KeyringId::kPolkadotMainnet, "DOT"},
  };

  for (const auto& test_case : test_cases) {
    auto params = mojom::SwapQuoteParams::New();
    params->from_account_id = mojom::AccountId::New(
        test_case.coin, test_case.keyring, mojom::AccountKind::kDerived,
        "test_address", 0, "test_unique_key");
    params->to_account_id = mojom::AccountId::New(
        test_case.coin, test_case.keyring, mojom::AccountKind::kDerived,
        "test_address", 0, "test_unique_key");
    params->from_chain_id = "test_chain";
    params->from_token = "test_token";
    params->from_amount = "1000000";
    params->to_chain_id = "test_chain";
    params->to_token = "test_token";
    params->slippage_percentage = "0.5";
    params->route_priority = mojom::RoutePriority::kCheapest;
    params->provider = mojom::SwapProvider::kNearIntents;

    auto encoded_params = gate3::EncodeQuoteParams(std::move(params));
    ASSERT_NE(encoded_params, std::nullopt)
        << "Failed for coin type: " << test_case.expected_coin_string;

    auto parsed = ParseJson(*encoded_params);
    ASSERT_TRUE(parsed.is_dict());

    auto* source_coin = parsed.GetDict().FindString("sourceCoin");
    ASSERT_NE(source_coin, nullptr);
    EXPECT_EQ(*source_coin, test_case.expected_coin_string);

    auto* dest_coin = parsed.GetDict().FindString("destinationCoin");
    ASSERT_NE(dest_coin, nullptr);
    EXPECT_EQ(*dest_coin, test_case.expected_coin_string);
  }
}

TEST(SwapRequestHelperUnitTest, EncodeGate3StatusParams) {
  auto params = mojom::Gate3SwapStatusParams::New();
  params->route_id = "route-123-abc";
  params->tx_hash = "0xdeadbeef1234567890";
  params->source_coin = mojom::CoinType::ETH;
  params->source_chain_id = "0x1";
  params->destination_coin = mojom::CoinType::SOL;
  params->destination_chain_id = "0x65";
  params->deposit_address = "0xDepositAddress";
  params->deposit_memo = std::nullopt;
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded = gate3::EncodeStatusParams(std::move(params));
  ASSERT_TRUE(encoded.has_value());

  std::string expected = R"({
    "routeId": "route-123-abc",
    "txHash": "0xdeadbeef1234567890",
    "sourceCoin": "ETH",
    "sourceChainId": "0x1",
    "destinationCoin": "SOL",
    "destinationChainId": "0x65",
    "depositAddress": "0xDepositAddress",
    "depositMemo": "",
    "provider": "NEAR_INTENTS"
  })";
  EXPECT_EQ(ParseJson(*encoded), ParseJson(expected));
}

TEST(SwapRequestHelperUnitTest, EncodeGate3StatusParamsCrossChainZecToEth) {
  auto params = mojom::Gate3SwapStatusParams::New();
  params->route_id = "zec-eth-route";
  params->tx_hash = "zec-tx-hash";
  params->source_coin = mojom::CoinType::ZEC;
  params->source_chain_id = mojom::kZCashMainnet;
  params->destination_coin = mojom::CoinType::ETH;
  params->destination_chain_id = mojom::kMainnetChainId;
  params->deposit_address = "t1deposit";
  params->deposit_memo =
      std::vector<uint8_t>{'m', 'e', 'm', 'o', '1', '2', '3'};
  params->provider = mojom::SwapProvider::kNearIntents;

  auto encoded = gate3::EncodeStatusParams(std::move(params));
  ASSERT_TRUE(encoded.has_value());

  auto parsed = ParseJson(*encoded);
  ASSERT_TRUE(parsed.is_dict());
  EXPECT_EQ(*parsed.GetDict().FindString("sourceCoin"), "ZEC");
  EXPECT_EQ(*parsed.GetDict().FindString("destinationCoin"), "ETH");
  EXPECT_EQ(*parsed.GetDict().FindString("depositMemo"), "memo123");
}

}  // namespace brave_wallet
