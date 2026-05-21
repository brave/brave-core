/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_response_parser.h"

#include <memory>
#include <utility>

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

const char* GetJupiterQuoteResponse() {
  return R"(
    {
      "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "1000000",
      "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
      "outAmount": "781469842",
      "otherAmountThreshold": "781391696",
      "swapMode": "ExactIn",
      "slippageBps": "1",
      "platformFee": {
        "amount": "6775397",
        "feeBps": "85"
      },
      "priceImpactPct": "0",
      "routePlan": [
        {
          "swapInfo": {
            "ammKey": "HCk6LA93xPVsF8g4v6gjkiCd88tLXwZq4eJwiYNHR8da",
            "label": "Raydium",
            "inputMint": "So11111111111111111111111111111111111111112",
            "outputMint": "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4",
            "inAmount": "997500",
            "outAmount": "4052482154",
            "feeAmount": "2500",
            "feeMint": "So11111111111111111111111111111111111111112"
          },
          "percent": "100"
        },
        {
          "swapInfo": {
            "ammKey": "HqrRmb2MbEiTrJS5KXhDzUoKbSLbBXJvhNBGEyDNo9Tr",
            "label": "Meteora",
            "inputMint": "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4",
            "outputMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS",
            "inAmount": "4052482154",
            "outAmount": "834185227",
            "feeAmount": "10131205",
            "feeMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS"
          },
          "percent": "100"
        },
        {
          "swapInfo": {
            "ammKey": "6shkv2VNBPWVABvShgcGmrv98Z1vR3EcdwND6XUwGoFq",
            "label": "Meteora",
            "inputMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS",
            "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
            "inAmount": "834185227",
            "outAmount": "781469842",
            "feeAmount": "2085463",
            "feeMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263"
          },
          "percent": "100"
        }
      ]
    })";
}

}  // namespace

TEST(SwapResponseParserUnitTest, ParseZeroExQuoteResponse) {
  // Case 1: non-null zeroExFee
  std::string json(R"(
    {
      "blockNumber": "20114676",
      "buyAmount": "100032748",
      "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "fees": {
        "integratorFee": null,
        "zeroExFee": {
          "amount": "0",
          "token": "0xdeadbeef",
          "type": "volume"
        },
        "gasFee": null
      },
      "gas": "288095",
      "gasPrice": "7062490000",
      "issues": {
        "allowance": {
          "actual": "0",
          "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
        },
        "balance": {
          "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
          "actual": "0",
          "expected": "100000000"
        },
        "simulationIncomplete": false,
        "invalidSourcesPassed": []
      },
      "liquidityAvailable": true,
      "minBuyAmount": "99032421",
      "route": {
        "fills": [
          {
            "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "source": "SolidlyV3",
            "proportionBps": "10000"
          }
        ],
        "tokens": [
          {
            "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "symbol": "USDC"
          },
          {
            "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "symbol": "USDT"
          }
        ]
      },
      "sellAmount": "100000000",
      "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "tokenMetadata": {
        "buyToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        },
        "sellToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        }
      },
      "totalNetworkFee": "2034668056550000",
      "zid": "0x111111111111111111111111"
    }
  )");
  auto quote =
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId);
  ASSERT_TRUE(quote);

  EXPECT_EQ(quote->buy_amount, "100032748");
  EXPECT_EQ(quote->buy_token, "0xdac17f958d2ee523a2206206994597c13d831ec7");

  ASSERT_TRUE(quote->fees->zero_ex_fee);
  EXPECT_EQ(quote->fees->zero_ex_fee->amount, "0");
  EXPECT_EQ(quote->fees->zero_ex_fee->token, "0xdeadbeef");
  EXPECT_EQ(quote->fees->zero_ex_fee->type, "volume");

  EXPECT_EQ(quote->gas, "288095");
  EXPECT_EQ(quote->gas_price, "7062490000");
  EXPECT_EQ(quote->liquidity_available, true);
  EXPECT_EQ(quote->min_buy_amount, "99032421");

  ASSERT_EQ(quote->route->fills.size(), 1UL);
  EXPECT_EQ(quote->route->fills.at(0)->from,
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(quote->route->fills.at(0)->to,
            "0xdac17f958d2ee523a2206206994597c13d831ec7");
  EXPECT_EQ(quote->route->fills.at(0)->source, "SolidlyV3");
  EXPECT_EQ(quote->route->fills.at(0)->proportion_bps, "10000");

  EXPECT_EQ(quote->sell_amount, "100000000");
  EXPECT_EQ(quote->sell_token, "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(quote->total_network_fee, "2034668056550000");

  EXPECT_EQ(quote->liquidity_available, true);
  EXPECT_EQ(quote->allowance_target,
            "0x0000000000001fF3684f28c67538d4D072C22734");

  // Case 2: null zeroExFee
  json = R"(
    {
      "blockNumber": "20114676",
      "buyAmount": "100032748",
      "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "fees": {
        "integratorFee": null,
        "zeroExFee": null,
        "gasFee": null
      },
      "gas": "288095",
      "gasPrice": "7062490000",
      "issues": {
        "allowance": {
          "actual": "0",
          "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
        },
        "balance": {
          "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
          "actual": "0",
          "expected": "100000000"
        },
        "simulationIncomplete": false,
        "invalidSourcesPassed": []
      },
      "liquidityAvailable": true,
      "minBuyAmount": "99032421",
      "route": {
        "fills": [
          {
            "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "source": "SolidlyV3",
            "proportionBps": "10000"
          }
        ],
        "tokens": [
          {
            "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "symbol": "USDC"
          },
          {
            "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "symbol": "USDT"
          }
        ]
      },
      "sellAmount": "100000000",
      "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "tokenMetadata": {
        "buyToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        },
        "sellToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        }
      },
      "totalNetworkFee": "2034668056550000",
      "zid": "0x111111111111111111111111"
    }
  )";
  quote = zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId);
  ASSERT_TRUE(quote);
  EXPECT_FALSE(quote->fees->zero_ex_fee);
  EXPECT_EQ(quote->liquidity_available, true);
  EXPECT_EQ(quote->allowance_target,
            "0x0000000000001fF3684f28c67538d4D072C22734");

  // Case 3: malformed fees field
  json = R"(
    {
      "blockNumber": "20114676",
      "buyAmount": "100032748",
      "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "fees": null,
      "gas": "288095",
      "gasPrice": "7062490000",
      "issues": {
        "allowance": {
          "actual": "0",
          "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
        },
        "balance": {
          "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
          "actual": "0",
          "expected": "100000000"
        },
        "simulationIncomplete": false,
        "invalidSourcesPassed": []
      },
      "liquidityAvailable": true,
      "minBuyAmount": "99032421",
      "route": {
        "fills": [
          {
            "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "source": "SolidlyV3",
            "proportionBps": "10000"
          }
        ],
        "tokens": [
          {
            "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "symbol": "USDC"
          },
          {
            "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "symbol": "USDT"
          }
        ]
      },
      "sellAmount": "100000000",
      "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "tokenMetadata": {
        "buyToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        },
        "sellToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        }
      },
      "totalNetworkFee": "2034668056550000",
      "zid": "0x111111111111111111111111"
    }
  )";
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId));

  // Case 4: insufficient liquidity
  json = R"(
    {
      "liquidityAvailable": false,
    }
  )";
  quote = zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId);
  ASSERT_TRUE(quote);
  EXPECT_FALSE(quote->liquidity_available);
  EXPECT_EQ(quote->buy_token, "");

  // Case 5: other invalid cases
  json = R"({"totalNetworkFee": "2034668056550000"})";
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId));
  json = R"({"totalNetworkFee": 2034668056550000})";
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId));
  json = "3";
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId));
  json = "[3]";
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(ParseJson(json), mojom::kMainnetChainId));
  EXPECT_FALSE(
      zeroex::ParseQuoteResponse(base::Value(), mojom::kMainnetChainId));
}

TEST(SwapResponseParserUnitTest, ParseZeroExTransactionResponse) {
  // Case 1: valid transaction
  std::string json(R"(
    {
      "transaction": {
        "to": "0x7f6cee965959295cc64d0e6c00d99d6532d8e86b",
        "data": "0xdeadbeef",
        "gas": "288079",
        "gasPrice": "4837860000",
        "value": "0"
      }
    }
  )");
  mojom::ZeroExTransactionPtr transaction =
      zeroex::ParseTransactionResponse(ParseJson(json));
  ASSERT_TRUE(transaction);

  EXPECT_EQ(transaction->to, "0x7f6cee965959295cc64d0e6c00d99d6532d8e86b");
  EXPECT_EQ(transaction->data, "0xdeadbeef");
  EXPECT_EQ(transaction->gas, "288079");
  EXPECT_EQ(transaction->gas_price, "4837860000");
  EXPECT_EQ(transaction->value, "0");

  // Case 2: invalid cases
  json = "3";
  EXPECT_FALSE(zeroex::ParseTransactionResponse(ParseJson(json)));
  json = "[3]";
  EXPECT_FALSE(zeroex::ParseTransactionResponse(ParseJson(json)));
  EXPECT_FALSE(zeroex::ParseTransactionResponse(base::Value()));
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuoteResponse) {
  auto* json = GetJupiterQuoteResponse();
  mojom::JupiterQuotePtr swap_quote =
      jupiter::ParseQuoteResponse(ParseJson(json));

  ASSERT_TRUE(swap_quote);
  EXPECT_EQ(swap_quote->input_mint,
            "So11111111111111111111111111111111111111112");
  EXPECT_EQ(swap_quote->in_amount, "1000000");
  EXPECT_EQ(swap_quote->output_mint,
            "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263");
  EXPECT_EQ(swap_quote->out_amount, "781469842");
  EXPECT_EQ(swap_quote->other_amount_threshold, "781391696");
  EXPECT_EQ(swap_quote->swap_mode, "ExactIn");
  EXPECT_EQ(swap_quote->slippage_bps, "1");
  EXPECT_EQ(swap_quote->price_impact_pct, "0");

  ASSERT_TRUE(swap_quote->platform_fee);
  EXPECT_EQ(swap_quote->platform_fee->amount, "6775397");
  EXPECT_EQ(swap_quote->platform_fee->fee_bps, "85");

  ASSERT_EQ(swap_quote->route_plan.size(), 3UL);
  EXPECT_EQ(swap_quote->route_plan.at(0)->percent, "100");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->amm_key,
            "HCk6LA93xPVsF8g4v6gjkiCd88tLXwZq4eJwiYNHR8da");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->label, "Raydium");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->input_mint,
            "So11111111111111111111111111111111111111112");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->output_mint,
            "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->in_amount, "997500");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->out_amount, "4052482154");
  ASSERT_TRUE(swap_quote->route_plan.at(0)->swap_info->fee_amount);
  EXPECT_EQ(*swap_quote->route_plan.at(0)->swap_info->fee_amount, "2500");
  ASSERT_TRUE(swap_quote->route_plan.at(0)->swap_info->fee_mint);
  EXPECT_EQ(*swap_quote->route_plan.at(0)->swap_info->fee_mint,
            "So11111111111111111111111111111111111111112");
  EXPECT_EQ(swap_quote->route_plan.at(1)->percent, "100");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->amm_key,
            "HqrRmb2MbEiTrJS5KXhDzUoKbSLbBXJvhNBGEyDNo9Tr");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->label, "Meteora");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->input_mint,
            "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->output_mint,
            "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->in_amount, "4052482154");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->out_amount, "834185227");
  ASSERT_TRUE(swap_quote->route_plan.at(1)->swap_info->fee_amount);
  EXPECT_EQ(*swap_quote->route_plan.at(1)->swap_info->fee_amount, "10131205");
  ASSERT_TRUE(swap_quote->route_plan.at(1)->swap_info->fee_mint);
  EXPECT_EQ(*swap_quote->route_plan.at(1)->swap_info->fee_mint,
            "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS");
  EXPECT_EQ(swap_quote->route_plan.at(2)->percent, "100");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->amm_key,
            "6shkv2VNBPWVABvShgcGmrv98Z1vR3EcdwND6XUwGoFq");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->label, "Meteora");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->input_mint,
            "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->output_mint,
            "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->in_amount, "834185227");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->out_amount, "781469842");
  ASSERT_TRUE(swap_quote->route_plan.at(2)->swap_info->fee_amount);
  EXPECT_EQ(*swap_quote->route_plan.at(2)->swap_info->fee_amount, "2085463");
  ASSERT_TRUE(swap_quote->route_plan.at(2)->swap_info->fee_mint);
  EXPECT_EQ(*swap_quote->route_plan.at(2)->swap_info->fee_mint,
            "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263");

  // OK: missing optional feeAmount and feeMint fields
  auto quote_without_fees = jupiter::ParseQuoteResponse(ParseJson(R"({
    "inputMint": "So11111111111111111111111111111111111111112",
    "inAmount": "1000000",
    "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
    "outAmount": "781469842",
    "otherAmountThreshold": "781391696",
    "swapMode": "ExactIn",
    "slippageBps": "1",
    "platformFee": null,
    "priceImpactPct": "0",
    "routePlan": [
      {
        "swapInfo": {
          "ammKey": "HCk6LA93xPVsF8g4v6gjkiCd88tLXwZq4eJwiYNHR8da",
          "label": "Raydium",
          "inputMint": "So11111111111111111111111111111111111111112",
          "outputMint": "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4",
          "inAmount": "997500",
          "outAmount": "4052482154"
        },
        "percent": "100"
      }
    ]
  })"));
  ASSERT_TRUE(quote_without_fees);
  ASSERT_EQ(quote_without_fees->route_plan.size(), 1UL);
  EXPECT_FALSE(quote_without_fees->route_plan.at(0)->swap_info->fee_amount);
  EXPECT_FALSE(quote_without_fees->route_plan.at(0)->swap_info->fee_mint);

  // OK: null platformFee value
  EXPECT_TRUE(jupiter::ParseQuoteResponse(ParseJson(R"({
    "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "1000000",
      "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
      "outAmount": "781469842",
      "otherAmountThreshold": "781391696",
      "swapMode": "ExactIn",
      "slippageBps": "1",
      "platformFee": null,
      "priceImpactPct": "0",
      "routePlan": []
  })")));

  // KO: Malformed quote
  EXPECT_FALSE(jupiter::ParseQuoteResponse(base::Value()));

  // KO: Invalid quote
  EXPECT_FALSE(jupiter::ParseQuoteResponse(ParseJson(R"({"price": "3"})")));

  // KO: Invalid platformFee value
  EXPECT_FALSE(jupiter::ParseQuoteResponse(ParseJson(R"({
    "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "1000000",
      "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
      "outAmount": "781469842",
      "otherAmountThreshold": "781391696",
      "swapMode": "ExactIn",
      "slippageBps": "1",
      "platformFee": "foo",
      "priceImpactPct": "0",
      "routePlan": []
  })")));
}

TEST(SwapResponseParserUnitTest, ParseJupiterTransactionResponse) {
  std::string json(R"(
    {
      "swapTransaction": "swap"
    })");

  auto transaction = jupiter::ParseTransactionResponse(ParseJson(json));
  ASSERT_TRUE(transaction);
  ASSERT_EQ(transaction, "swap");

  ASSERT_FALSE(jupiter::ParseTransactionResponse(base::Value()));
  ASSERT_FALSE(
      jupiter::ParseTransactionResponse(ParseJson(R"({"foo": "bar"})")));
}

TEST(SwapResponseParserUnitTest, ParseZeroExErrorResponse) {
  {
    std::string json(R"(
      {
        "name": "INPUT_INVALID",
        "message": "Validation Failed"
      })");

    auto swap_error = zeroex::ParseErrorResponse(ParseJson(json));
    EXPECT_EQ(swap_error->name, "INPUT_INVALID");
    EXPECT_EQ(swap_error->message, "Validation Failed");
  }
}

TEST(SwapResponseParserUnitTest, ParseJupiterErrorResponse) {
  {
    std::string json(R"(
    {
      "statusCode": "some code",
      "error": "error",
      "message": "No routes found for the input and output mints"
    })");

    auto jupiter_error = jupiter::ParseErrorResponse(ParseJson(json));
    EXPECT_EQ(jupiter_error->status_code, "some code");
    EXPECT_EQ(jupiter_error->error, "error");
    EXPECT_EQ(jupiter_error->message,
              "No routes found for the input and output mints");

    EXPECT_TRUE(jupiter_error->is_insufficient_liquidity);
  }
  {
    std::string json(R"(
    {
      "statusCode": "some code",
      "error": "error",
      "message": "some message"
    })");

    auto jupiter_error = jupiter::ParseErrorResponse(ParseJson(json));
    EXPECT_EQ(jupiter_error->status_code, "some code");
    EXPECT_EQ(jupiter_error->error, "error");
    EXPECT_EQ(jupiter_error->message, "some message");

    EXPECT_FALSE(jupiter_error->is_insufficient_liquidity);
  }
}

TEST(SwapResponseParserUnitTest, ParseGate3QuoteResponse) {
  // Indicative quote response from Gate3 API
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_da99b36c884a",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "sourceAmount": "1000000",
              "destinationToken": {
                "coin": "SOL",
                "chainId": "0x65",
                "contractAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "destinationAmount": "714449",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://static1.tokenterminal.com/near/products/nearintents/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "1000000",
          "destinationAmount": "714449",
          "destinationAmountMin": "710876",
          "estimatedTime": "42",
          "priceImpact": "-28.561424569827942",
          "networkFee": null,
          "gasless": false,
          "depositAddress": null,
          "depositMemo": null,
          "expiresAt": null,
          "slippagePercentage": "0.5",
          "transactionParams": null,
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_da99b36c884a");
  EXPECT_EQ(route->provider, mojom::SwapProvider::kNearIntents);
  EXPECT_EQ(route->source_amount, "1000000");
  EXPECT_EQ(route->destination_amount, "714449");
  EXPECT_EQ(route->destination_amount_min, "710876");
  ASSERT_TRUE(route->estimated_time.has_value());
  EXPECT_EQ(*route->estimated_time, "42");
  ASSERT_TRUE(route->price_impact.has_value());
  EXPECT_EQ(*route->price_impact, "-28.561424569827942");
  EXPECT_FALSE(route->deposit_address.has_value());
  EXPECT_FALSE(route->deposit_memo.has_value());
  EXPECT_FALSE(route->expires_at.has_value());
  EXPECT_FALSE(route->requires_token_allowance);
  EXPECT_TRUE(route->requires_firm_route);
  EXPECT_FALSE(route->transaction_params);

  // Verify steps
  ASSERT_EQ(route->steps.size(), 1u);
  const auto& step = route->steps[0];
  EXPECT_EQ(step->source_amount, "1000000");
  EXPECT_EQ(step->destination_amount, "714449");

  // Verify source token
  EXPECT_EQ(step->source_token->coin, mojom::CoinType::ETH);
  EXPECT_EQ(step->source_token->chain_id, "0x1");
  EXPECT_EQ(step->source_token->contract_address,
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(step->source_token->symbol, "USDC");
  EXPECT_EQ(step->source_token->logo, "");

  // Verify destination token
  EXPECT_EQ(step->destination_token->coin, mojom::CoinType::SOL);
  EXPECT_EQ(step->destination_token->chain_id, "0x65");
  EXPECT_EQ(step->destination_token->contract_address,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  EXPECT_EQ(step->destination_token->symbol, "USDC");
  EXPECT_EQ(step->destination_token->logo, "");

  // Verify tool
  EXPECT_EQ(step->tool->name, "NEAR Intents");
  EXPECT_EQ(
      step->tool->logo,
      "https://static1.tokenterminal.com/near/products/nearintents/logo.png");
}

TEST(SwapResponseParserUnitTest, ParseGate3QuoteResponseWithEvmTransaction) {
  // Firm quote response with EVM transaction params
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_e6df68df401c",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "sourceAmount": "1000000",
              "destinationToken": {
                "coin": "SOL",
                "chainId": "0x65",
                "contractAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "destinationAmount": "714488",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://static1.tokenterminal.com/near/products/nearintents/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "1000000",
          "destinationAmount": "714488",
          "destinationAmountMin": "710915",
          "estimatedTime": "42",
          "priceImpact": "-28.551420568227304",
          "networkFee": {
            "amount": "21000000000000",
            "decimals": "18",
            "symbol": "ETH"
          },
          "gasless": false,
          "depositAddress": "0x16a0FdeB69D821753440dFA092316F54eF95E967",
          "depositMemo": null,
          "expiresAt": "1767810375",
          "slippagePercentage": "0.5",
          "transactionParams": {
            "evm": {
              "chain": {
                "coin": "ETH",
                "chainId": "0x1"
              },
              "from": "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045",
              "to": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
              "value": "0",
              "data": "0xa9059cbb00000000000000000000000016a0fdeb69d821753440dfa092316f54ef95e96700000000000000000000000000000000000000000000000000000000000f4240",
              "gasLimit": "0xfde8",
              "gasPrice": null
            },
            "solana": null,
            "bitcoin": null,
            "cardano": null,
            "zcash": null
          },
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_e6df68df401c");
  EXPECT_EQ(route->provider, mojom::SwapProvider::kNearIntents);
  EXPECT_EQ(route->source_amount, "1000000");
  EXPECT_EQ(route->destination_amount, "714488");
  EXPECT_EQ(route->destination_amount_min, "710915");
  ASSERT_TRUE(route->deposit_address.has_value());
  EXPECT_EQ(*route->deposit_address,
            "0x16a0FdeB69D821753440dFA092316F54eF95E967");
  ASSERT_TRUE(route->expires_at.has_value());
  EXPECT_EQ(*route->expires_at, "1767810375");

  // Verify network fee
  ASSERT_TRUE(route->network_fee);
  EXPECT_EQ(route->network_fee->amount, "21000000000000");
  EXPECT_EQ(route->network_fee->decimals, 18);
  EXPECT_EQ(route->network_fee->symbol, "ETH");

  // Verify EVM transaction params
  ASSERT_TRUE(route->transaction_params);
  ASSERT_TRUE(route->transaction_params->is_evm_transaction_params());

  const auto& evm_params =
      route->transaction_params->get_evm_transaction_params();
  EXPECT_EQ(evm_params->chain->coin, mojom::CoinType::ETH);
  EXPECT_EQ(evm_params->chain->chain_id, "0x1");
  EXPECT_EQ(evm_params->from, "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");
  EXPECT_EQ(evm_params->to, "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(evm_params->value, "0");
  EXPECT_EQ(evm_params->data,
            "0xa9059cbb00000000000000000000000016a0fdeb69d821753440dfa092316f54"
            "ef95e967"
            "00000000000000000000000000000000000000000000000000000000000f4240");
}

TEST(SwapResponseParserUnitTest, ParseGate3QuoteResponseWithSolanaTransaction) {
  // Firm quote response with Solana transaction params
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_sol_test",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "SOL",
                "chainId": "0x65",
                "contractAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "sourceAmount": "1000000",
              "destinationToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
                "symbol": "USDC",
                "decimals": "6",
                "logo": null
              },
              "destinationAmount": "990000",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://example.com/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "1000000",
          "destinationAmount": "990000",
          "destinationAmountMin": "980000",
          "estimatedTime": "60",
          "priceImpact": "-1.5",
          "networkFee": {
            "amount": "5000",
            "decimals": "9",
            "symbol": "SOL"
          },
          "gasless": false,
          "depositAddress": "DepositAddress123",
          "depositMemo": null,
          "expiresAt": "1767810375",
          "slippagePercentage": "0.5",
          "transactionParams": {
            "evm": null,
            "solana": {
              "chain": {
                "coin": "SOL",
                "chainId": "0x65"
              },
              "from": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
              "to": "DepositAddress123",
              "lamports": "0",
              "splTokenMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "splTokenAmount": "1000000",
              "decimals": "6",
              "versionedTransaction": null,
              "computeUnitLimit": null,
              "computeUnitPrice": null
            },
            "bitcoin": null,
            "cardano": null,
            "zcash": null
          },
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_sol_test");

  // Verify network fee
  ASSERT_TRUE(route->network_fee);
  EXPECT_EQ(route->network_fee->amount, "5000");
  EXPECT_EQ(route->network_fee->decimals, 9);
  EXPECT_EQ(route->network_fee->symbol, "SOL");

  // Verify Solana transaction params
  ASSERT_TRUE(route->transaction_params);
  ASSERT_TRUE(route->transaction_params->is_solana_transaction_params());

  const auto& sol_params =
      route->transaction_params->get_solana_transaction_params();
  EXPECT_EQ(sol_params->chain->coin, mojom::CoinType::SOL);
  EXPECT_EQ(sol_params->chain->chain_id, "0x65");
  EXPECT_EQ(sol_params->from, "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  EXPECT_EQ(sol_params->to, "DepositAddress123");
  EXPECT_EQ(sol_params->lamports, "0");
  ASSERT_TRUE(sol_params->spl_token_mint.has_value());
  EXPECT_EQ(*sol_params->spl_token_mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  ASSERT_TRUE(sol_params->spl_token_amount.has_value());
  EXPECT_EQ(*sol_params->spl_token_amount, "1000000");
  ASSERT_TRUE(sol_params->decimals.has_value());
  EXPECT_EQ(*sol_params->decimals, "6");
}

TEST(SwapResponseParserUnitTest,
     ParseGate3QuoteResponseWithBitcoinTransaction) {
  // Firm quote response with Bitcoin transaction params
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_btc_test",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "BTC",
                "chainId": "bitcoin_mainnet",
                "contractAddress": null,
                "symbol": "BTC",
                "decimals": "8",
                "logo": null
              },
              "sourceAmount": "100000",
              "destinationToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "symbol": "ETH",
                "decimals": "18",
                "logo": null
              },
              "destinationAmount": "50000000000000000",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://example.com/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "100000",
          "destinationAmount": "50000000000000000",
          "destinationAmountMin": "49500000000000000",
          "estimatedTime": "600",
          "priceImpact": "-0.5",
          "networkFee": null,
          "gasless": false,
          "depositAddress": "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
          "depositMemo": null,
          "expiresAt": "1767810375",
          "slippagePercentage": "0.5",
          "transactionParams": {
            "evm": null,
            "solana": null,
            "bitcoin": {
              "chain": {
                "coin": "BTC",
                "chainId": "bitcoin_mainnet"
              },
              "to": "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
              "value": "100000",
              "refundTo": "bc1sender123456789"
            },
            "cardano": null,
            "zcash": null
          },
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_btc_test");

  // Verify Bitcoin transaction params
  ASSERT_TRUE(route->transaction_params);
  ASSERT_TRUE(route->transaction_params->is_bitcoin_transaction_params());

  const auto& btc_params =
      route->transaction_params->get_bitcoin_transaction_params();
  EXPECT_EQ(btc_params->chain->coin, mojom::CoinType::BTC);
  EXPECT_EQ(btc_params->chain->chain_id, "bitcoin_mainnet");
  EXPECT_EQ(btc_params->to, "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh");
  EXPECT_EQ(btc_params->value, "100000");
  EXPECT_EQ(btc_params->refund_to, "bc1sender123456789");
}

TEST(SwapResponseParserUnitTest,
     ParseGate3QuoteResponseWithCardanoTransaction) {
  // Firm quote response with Cardano transaction params
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_ada_test",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "ADA",
                "chainId": "cardano_mainnet",
                "contractAddress": null,
                "symbol": "ADA",
                "decimals": "6",
                "logo": null
              },
              "sourceAmount": "1000000",
              "destinationToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "symbol": "ETH",
                "decimals": "18",
                "logo": null
              },
              "destinationAmount": "50000000000000000",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://example.com/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "1000000",
          "destinationAmount": "50000000000000000",
          "destinationAmountMin": "49500000000000000",
          "estimatedTime": "600",
          "priceImpact": "-0.5",
          "networkFee": null,
          "gasless": false,
          "depositAddress": "addr1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
          "depositMemo": null,
          "expiresAt": "1767810375",
          "slippagePercentage": "0.5",
          "transactionParams": {
            "evm": null,
            "solana": null,
            "bitcoin": null,
            "cardano": {
              "chain": {
                "coin": "ADA",
                "chainId": "cardano_mainnet"
              },
              "to": "addr1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
              "value": "1000000",
              "refundTo": "addr1sender123456789"
            },
            "zcash": null
          },
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_ada_test");

  // Verify Cardano transaction params
  ASSERT_TRUE(route->transaction_params);
  ASSERT_TRUE(route->transaction_params->is_cardano_transaction_params());

  const auto& cardano_params =
      route->transaction_params->get_cardano_transaction_params();
  EXPECT_EQ(cardano_params->chain->coin, mojom::CoinType::ADA);
  EXPECT_EQ(cardano_params->chain->chain_id, "cardano_mainnet");
  EXPECT_EQ(cardano_params->to, "addr1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh");
  EXPECT_EQ(cardano_params->value, "1000000");
  EXPECT_EQ(cardano_params->refund_to, "addr1sender123456789");
}

TEST(SwapResponseParserUnitTest, ParseGate3QuoteResponseWithZCashTransaction) {
  // Firm quote response with ZCash transaction params
  std::string json(R"(
    {
      "routes": [
        {
          "id": "ni_zec_test",
          "provider": "NEAR_INTENTS",
          "steps": [
            {
              "sourceToken": {
                "coin": "ZEC",
                "chainId": "zcash_mainnet",
                "contractAddress": null,
                "symbol": "ZEC",
                "decimals": "8",
                "logo": null
              },
              "sourceAmount": "10000000",
              "destinationToken": {
                "coin": "ETH",
                "chainId": "0x1",
                "contractAddress": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "symbol": "ETH",
                "decimals": "18",
                "logo": null
              },
              "destinationAmount": "50000000000000000",
              "tool": {
                "name": "NEAR Intents",
                "logo": "https://example.com/logo.png"
              },
              "percent": "100"
            }
          ],
          "sourceAmount": "10000000",
          "destinationAmount": "50000000000000000",
          "destinationAmountMin": "49500000000000000",
          "estimatedTime": "600",
          "priceImpact": "-0.5",
          "networkFee": null,
          "gasless": false,
          "depositAddress": "t1dJj1rGjG7GjG7GjG7GjG7GjG7GjG7GjG7G",
          "depositMemo": null,
          "expiresAt": "1767810375",
          "slippagePercentage": "0.5",
          "transactionParams": {
            "evm": null,
            "solana": null,
            "bitcoin": null,
            "cardano": null,
            "zcash": {
              "chain": {
                "coin": "ZEC",
                "chainId": "zcash_mainnet"
              },
              "to": "t1dJj1rGjG7GjG7GjG7GjG7GjG7GjG7GjG7G",
              "value": "10000000",
              "refundTo": "t1sender123456789"
            }
          },
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");

  auto quote = gate3::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1u);

  const auto& route = quote->routes[0];
  EXPECT_EQ(route->id, "ni_zec_test");

  // Verify ZCash transaction params
  ASSERT_TRUE(route->transaction_params);
  ASSERT_TRUE(route->transaction_params->is_zcash_transaction_params());

  const auto& zcash_params =
      route->transaction_params->get_zcash_transaction_params();
  EXPECT_EQ(zcash_params->chain->coin, mojom::CoinType::ZEC);
  EXPECT_EQ(zcash_params->chain->chain_id, "zcash_mainnet");
  EXPECT_EQ(zcash_params->to, "t1dJj1rGjG7GjG7GjG7GjG7GjG7GjG7GjG7G");
  EXPECT_EQ(zcash_params->value, "10000000");
  EXPECT_EQ(zcash_params->refund_to, "t1sender123456789");
}

TEST(SwapResponseParserUnitTest, ParseGate3ErrorResponse) {
  std::string json(R"(
    {
      "message": "Provider NEAR_INTENTS does not support this swap",
      "kind": "UNKNOWN"
    }
  )");

  auto error = gate3::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->message, "Provider NEAR_INTENTS does not support this swap");
  EXPECT_EQ(error->kind, mojom::Gate3SwapErrorKind::kUnknown);
}

TEST(SwapResponseParserUnitTest, ParseGate3ErrorResponseInsufficientLiquidity) {
  std::string json(R"(
    {
      "message": "No liquidity available for this swap",
      "kind": "INSUFFICIENT_LIQUIDITY"
    }
  )");

  auto error = gate3::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->message, "No liquidity available for this swap");
  EXPECT_EQ(error->kind, mojom::Gate3SwapErrorKind::kInsufficientLiquidity);
}

TEST(SwapResponseParserUnitTest, ParseGate3QuoteResponseInvalidJson) {
  // Empty routes
  std::string json_empty_routes(R"({"routes": []})");
  auto quote = gate3::ParseQuoteResponse(ParseJson(json_empty_routes));
  EXPECT_FALSE(quote);

  // Missing routes field
  std::string json_no_routes(R"({})");
  quote = gate3::ParseQuoteResponse(ParseJson(json_no_routes));
  EXPECT_FALSE(quote);

  // Invalid provider
  std::string json_invalid_provider(R"(
    {
      "routes": [
        {
          "id": "test",
          "provider": "INVALID_PROVIDER",
          "steps": [],
          "sourceAmount": "1000000",
          "destinationAmount": "714449",
          "destinationAmountMin": "710876",
          "requiresTokenAllowance": false,
          "requiresFirmRoute": true
        }
      ]
    }
  )");
  quote = gate3::ParseQuoteResponse(ParseJson(json_invalid_provider));
  EXPECT_FALSE(quote);
}

TEST(SwapResponseParserUnitTest, ParseGate3StatusResponse) {
  struct TestCase {
    std::string status_string;
    std::string internal_status;
    std::string explorer_url;
    mojom::Gate3SwapStatusCode expected_code;
  };
  std::vector<TestCase> cases = {
      {"PENDING", "awaiting_deposit", "", mojom::Gate3SwapStatusCode::kPending},
      {"PROCESSING", "swap_in_progress", "https://explorer.example.com/tx/123",
       mojom::Gate3SwapStatusCode::kProcessing},
      {"SUCCESS", "completed", "https://solscan.io/tx/abc123",
       mojom::Gate3SwapStatusCode::kSuccess},
      {"FAILED", "swap_failed", "", mojom::Gate3SwapStatusCode::kFailed},
      {"REFUNDED", "refund_complete", "https://etherscan.io/tx/refund123",
       mojom::Gate3SwapStatusCode::kRefunded},
  };

  for (const auto& tc : cases) {
    std::string json =
        absl::StrFormat(R"({
      "status": "%s",
      "internalStatus": "%s",
      "explorerUrl": "%s"
    })",
                        tc.status_string, tc.internal_status, tc.explorer_url);

    auto status = gate3::ParseStatusResponse(ParseJson(json));
    ASSERT_TRUE(status) << "Failed to parse status: " << tc.status_string;
    EXPECT_EQ(status->status, tc.expected_code);
    EXPECT_EQ(status->internal_status, tc.internal_status);
    EXPECT_EQ(status->explorer_url, tc.explorer_url);
  }
}

TEST(SwapResponseParserUnitTest, ParseGate3StatusResponseInvalidJson) {
  // Missing status field
  std::string json_no_status = R"({
    "internalStatus": "test",
    "explorerUrl": ""
  })";
  EXPECT_FALSE(gate3::ParseStatusResponse(ParseJson(json_no_status)));

  // Empty JSON
  EXPECT_FALSE(gate3::ParseStatusResponse(ParseJson("{}")));

  // Invalid status value
  std::string json_invalid_status = R"({
    "status": "INVALID_STATUS",
    "internalStatus": "test",
    "explorerUrl": ""
  })";
  auto status = gate3::ParseStatusResponse(ParseJson(json_invalid_status));
  EXPECT_FALSE(status);
}

TEST(SwapResponseParserUnitTest,
     ParseGate3StatusResponseExplorerUrlValidation) {
  auto make_json = [](const std::string& url) {
    return absl::StrFormat(R"({
      "status": "SUCCESS",
      "internalStatus": "completed",
      "explorerUrl": "%s"
    })",
                           url);
  };

  // Valid HTTPS URL is accepted.
  {
    auto status = gate3::ParseStatusResponse(
        ParseJson(make_json("https://etherscan.io/tx/0x123")));
    ASSERT_TRUE(status);
    EXPECT_EQ(status->explorer_url, "https://etherscan.io/tx/0x123");
  }

  // javascript: URI is rejected.
  {
    auto status =
        gate3::ParseStatusResponse(ParseJson(make_json("javascript:alert(1)")));
    ASSERT_TRUE(status);
    EXPECT_TRUE(status->explorer_url.empty());
  }

  // data: URI is rejected.
  {
    auto status = gate3::ParseStatusResponse(
        ParseJson(make_json("data:text/html,<script>alert(1)</script>")));
    ASSERT_TRUE(status);
    EXPECT_TRUE(status->explorer_url.empty());
  }

  // HTTP URL is rejected (must be HTTPS).
  {
    auto status = gate3::ParseStatusResponse(
        ParseJson(make_json("http://etherscan.io/tx/0x123")));
    ASSERT_TRUE(status);
    EXPECT_TRUE(status->explorer_url.empty());
  }

  // Invalid URL is rejected.
  {
    auto status = gate3::ParseStatusResponse(ParseJson(make_json("not-a-url")));
    ASSERT_TRUE(status);
    EXPECT_TRUE(status->explorer_url.empty());
  }
}

}  // namespace brave_wallet
