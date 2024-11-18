/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->fee_amount, "2500");
  EXPECT_EQ(swap_quote->route_plan.at(0)->swap_info->fee_mint,
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
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->fee_amount, "10131205");
  EXPECT_EQ(swap_quote->route_plan.at(1)->swap_info->fee_mint,
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
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->fee_amount, "2085463");
  EXPECT_EQ(swap_quote->route_plan.at(2)->swap_info->fee_mint,
            "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263");

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

TEST(SwapResponseParserUnitTest, ParseLiFiQuoteResponse) {
  {
    // OK: valid quote
    std::string json(R"(
    {
      "routes": [
        {
          "id": "0x343bc553146a3dd8438518d80bd1b6957f3fec05d137ff65a940bfb5390d3f1f",
          "fromChainId": "1",
          "fromAmountUSD": "1.00",
          "fromAmount": "1000000",
          "fromToken": {
            "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
            "chainId": "1",
            "symbol": "USDC",
            "decimals": "6",
            "name": "USD Coin",
            "coinKey": "USDC",
            "logoURI": "usdc.png",
            "priceUSD": "1"
          },
          "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
          "toChainId": "10",
          "toAmountUSD": "1.01",
          "toAmount": "1013654",
          "toAmountMin": "1008586",
          "toToken": {
            "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
            "chainId": "10",
            "symbol": "USDT",
            "decimals": "6",
            "name": "USDT",
            "coinKey": "USDT",
            "logoURI": "usdt.png",
            "priceUSD": "0.999685"
          },
          "toAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
          "gasCostUSD": "14.65",
          "containsSwitchChain": false,
          "steps": [
            {
              "type": "lifi",
              "id": "5a6876f1-988e-4710-85b7-be2bd7681421",
              "tool": "optimism",
              "toolDetails": {
                "key": "optimism",
                "name": "Optimism Gateway",
                "logoURI": "optimism.png"
              },
              "action": {
                "fromToken": {
                  "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                  "chainId": "1",
                  "symbol": "USDC",
                  "decimals": "6",
                  "name": "USD Coin",
                  "coinKey": "USDC",
                  "logoURI": "usdc.png",
                  "priceUSD": "1"
                },
                "fromAmount": "1000000",
                "toToken": {
                  "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
                  "chainId": "10",
                  "symbol": "USDT",
                  "decimals": "6",
                  "name": "USDT",
                  "coinKey": "USDT",
                  "logoURI": "usdt.png",
                  "priceUSD": "0.999685"
                },
                "fromChainId": "1",
                "toChainId": "10",
                "slippage": "0.005",
                "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
                "toAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
              },
              "estimate": {
                "tool": "optimism",
                "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                "toAmountMin": "1008586",
                "toAmount": "1013654",
                "fromAmount": "1000000",
                "feeCosts": [
                  {
                    "name": "LP Fee",
                    "description": "Fee paid to the Liquidity Provider",
                    "token": {
                      "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                      "chainId": "1",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "1"
                    },
                    "amount": "3000",
                    "amountUSD": "0.01",
                    "percentage": "0.003",
                    "included": true
                  }
                ],
                "gasCosts": [
                  {
                    "type": "SEND",
                    "price": "17621901985",
                    "estimate": "375000",
                    "limit": "618000",
                    "amount": "6608213244375000",
                    "amountUSD": "14.65",
                    "token": {
                      "address": "0x0000000000000000000000000000000000000000",
                      "chainId": "1",
                      "symbol": "ETH",
                      "decimals": "18",
                      "name": "ETH",
                      "coinKey": "ETH",
                      "logoURI": "eth.png",
                      "priceUSD": "2216.770000000000000000"
                    }
                  }
                ],
                "executionDuration": "106.944",
                "fromAmountUSD": "1.00",
                "toAmountUSD": "1.01"
              },
              "includedSteps": [
                {
                  "id": "9ac4d9f1-9b72-463d-baf1-fcd8b09f8a8d",
                  "type": "swap",
                  "action": {
                    "fromChainId": "1",
                    "fromAmount": "1000000",
                    "fromToken": {
                      "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                      "chainId": "1",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "1"
                    },
                    "toChainId": "1",
                    "toToken": {
                      "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
                      "chainId": "1",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "slippage": "0.005"
                  },
                  "estimate": {
                    "tool": "verse-dex",
                    "fromAmount": "1000000",
                    "toAmount": "1013654",
                    "toAmountMin": "1008586",
                    "approvalAddress": "0xB4B0ea46Fe0E9e8EAB4aFb765b527739F2718671",
                    "executionDuration": "30",
                    "feeCosts": [
                      {
                        "name": "LP Fee",
                        "description": "Fee paid to the Liquidity Provider",
                        "token": {
                          "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                          "chainId": "1",
                          "symbol": "USDC",
                          "decimals": "6",
                          "name": "USD Coin",
                          "coinKey": "USDC",
                          "logoURI": "usdc.png",
                          "priceUSD": "1"
                        },
                        "amount": "3000",
                        "amountUSD": "0.01",
                        "percentage": "0.003",
                        "included": true
                      }
                    ],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "17621901985",
                        "estimate": "200000",
                        "limit": "260000",
                        "amount": "3524380397000000",
                        "amountUSD": "7.81",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "1",
                          "symbol": "ETH",
                          "decimals": "18",
                          "name": "ETH",
                          "coinKey": "ETH",
                          "logoURI": "eth.png",
                          "priceUSD": "2216.770000000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "verse-dex",
                  "toolDetails": {
                    "key": "verse-dex",
                    "name": "Verse Dex",
                    "logoURI": "https://analytics.verse.bitcoin.com/logo.png"
                  }
                },
                {
                  "id": "b952ed38-1d5c-43bc-990a-468fd32d29b9",
                  "type": "cross",
                  "action": {
                    "fromChainId": "1",
                    "fromAmount": "1008586",
                    "fromToken": {
                      "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
                      "chainId": "1",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "toChainId": "10",
                    "toToken": {
                      "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
                      "chainId": "10",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "slippage": "0.005",
                    "destinationGasConsumption": "0",
                    "destinationCallData": "0x0"
                  },
                  "estimate": {
                    "tool": "optimism",
                    "fromAmount": "1013654",
                    "toAmount": "1013654",
                    "toAmountMin": "1008586",
                    "approvalAddress": "0x99C9fc46f92E8a1c0deC1b1747d010903E884bE1",
                    "executionDuration": "76.944",
                    "feeCosts": [],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "17621901985",
                        "estimate": "175000",
                        "limit": "227500",
                        "amount": "3083832847375000",
                        "amountUSD": "6.84",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "1",
                          "symbol": "ETH",
                          "decimals": "18",
                          "name": "ETH",
                          "coinKey": "ETH",
                          "logoURI": "eth.png",
                          "priceUSD": "2216.770000000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "optimism",
                  "toolDetails": {
                    "key": "optimism",
                    "name": "Optimism Gateway",
                    "logoURI": "optimism.png"
                  }
                }
              ],
              "integrator": "jumper.exchange"
            }
          ],
          "tags": [
            "RECOMMENDED",
            "CHEAPEST",
            "FASTEST"
          ]
        }
      ]
    })");

    auto quote = lifi::ParseQuoteResponse(ParseJson(json));
    ASSERT_TRUE(quote);
    ASSERT_EQ(quote->routes.size(), 1u);
    const auto& route = quote->routes.at(0);
    EXPECT_EQ(
        route->id,
        "0x343bc553146a3dd8438518d80bd1b6957f3fec05d137ff65a940bfb5390d3f1f");
    EXPECT_EQ(route->from_amount, "1000000");
    EXPECT_EQ(route->from_token->contract_address,
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48");
    EXPECT_EQ(route->from_token->chain_id, "0x1");
    EXPECT_EQ(route->from_token->symbol, "USDC");
    EXPECT_EQ(route->from_token->decimals, 6);
    EXPECT_EQ(route->from_token->name, "USD Coin");
    EXPECT_EQ(route->from_token->logo, "usdc.png");
    EXPECT_EQ(route->from_address,
              "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
    EXPECT_EQ(route->to_amount, "1013654");
    EXPECT_EQ(route->to_amount_min, "1008586");
    EXPECT_EQ(route->to_token->contract_address,
              "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58");
    EXPECT_EQ(route->to_token->chain_id, "0xa");
    EXPECT_EQ(route->to_token->symbol, "USDT");
    EXPECT_EQ(route->to_token->decimals, 6);
    EXPECT_EQ(route->to_token->name, "USDT");
    EXPECT_EQ(route->to_token->logo, "usdt.png");
    EXPECT_EQ(route->to_address, "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");

    ASSERT_EQ(route->steps.size(), 1u);
    const auto& step = route->steps.at(0);
    EXPECT_EQ(step->type, mojom::LiFiStepType::kLiFi);
    EXPECT_EQ(step->id, "5a6876f1-988e-4710-85b7-be2bd7681421");
    EXPECT_EQ(step->tool, "optimism");
    EXPECT_EQ(step->tool_details->key, "optimism");
    EXPECT_EQ(step->tool_details->name, "Optimism Gateway");
    EXPECT_EQ(step->tool_details->logo, "optimism.png");
    EXPECT_EQ(step->action->from_token->contract_address,
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48");
    EXPECT_EQ(step->action->from_token->chain_id, "0x1");
    EXPECT_EQ(step->action->from_token->symbol, "USDC");
    EXPECT_EQ(step->action->from_token->decimals, 6);
    EXPECT_EQ(step->action->from_token->name, "USD Coin");
    EXPECT_EQ(step->action->from_token->logo, "usdc.png");
    EXPECT_EQ(step->action->from_amount, "1000000");
    EXPECT_EQ(step->action->to_token->contract_address,
              "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58");
    EXPECT_EQ(step->action->to_token->chain_id, "0xa");
    EXPECT_EQ(step->action->to_token->symbol, "USDT");
    EXPECT_EQ(step->action->to_token->decimals, 6);
    EXPECT_EQ(step->action->to_token->name, "USDT");
    EXPECT_EQ(step->action->to_token->logo, "usdt.png");
    EXPECT_EQ(step->action->slippage, "0.005");
    EXPECT_EQ(step->action->from_address,
              "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
    EXPECT_EQ(step->action->to_address,
              "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
    EXPECT_EQ(step->estimate->tool, "optimism");
    EXPECT_EQ(step->estimate->approval_address,
              "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE");
    EXPECT_EQ(step->estimate->to_amount_min, "1008586");
    EXPECT_EQ(step->estimate->to_amount, "1013654");
    EXPECT_EQ(step->estimate->from_amount, "1000000");
    ASSERT_EQ(step->estimate->fee_costs->size(), 1u);
    const auto& fee_cost = step->estimate->fee_costs->at(0);
    EXPECT_EQ(fee_cost->name, "LP Fee");
    EXPECT_EQ(fee_cost->description, "Fee paid to the Liquidity Provider");
    EXPECT_EQ(fee_cost->token->contract_address,
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48");
    EXPECT_EQ(fee_cost->token->chain_id, "0x1");
    EXPECT_EQ(fee_cost->token->symbol, "USDC");
    EXPECT_EQ(fee_cost->token->decimals, 6);
    EXPECT_EQ(fee_cost->token->name, "USD Coin");
    EXPECT_EQ(fee_cost->token->logo, "usdc.png");
    EXPECT_EQ(fee_cost->amount, "3000");
    EXPECT_EQ(fee_cost->percentage, "0.003");
    EXPECT_TRUE(fee_cost->included);
    ASSERT_EQ(step->estimate->gas_costs.size(), 1u);
    const auto& gas_cost = step->estimate->gas_costs.at(0);
    EXPECT_EQ(gas_cost->type, "SEND");
    EXPECT_EQ(gas_cost->estimate, "375000");
    EXPECT_EQ(gas_cost->limit, "618000");
    EXPECT_EQ(gas_cost->amount, "6608213244375000");
    EXPECT_EQ(gas_cost->token->contract_address, "");
    EXPECT_EQ(gas_cost->token->chain_id, "0x1");
    EXPECT_EQ(gas_cost->token->symbol, "ETH");
    EXPECT_EQ(gas_cost->token->decimals, 18);
    EXPECT_EQ(gas_cost->token->name, "ETH");
    EXPECT_EQ(gas_cost->token->logo, "eth.png");
    EXPECT_EQ(step->estimate->execution_duration, "106.944");

    ASSERT_TRUE(step->included_steps);
    ASSERT_EQ(step->included_steps->size(), 2u);
    const auto& included_step_1 = step->included_steps->at(0);
    EXPECT_EQ(included_step_1->id, "9ac4d9f1-9b72-463d-baf1-fcd8b09f8a8d");
    EXPECT_EQ(included_step_1->type, mojom::LiFiStepType::kSwap);
    EXPECT_EQ(included_step_1->tool, "verse-dex");
    EXPECT_EQ(included_step_1->tool_details->key, "verse-dex");
    EXPECT_EQ(included_step_1->tool_details->name, "Verse Dex");
    EXPECT_EQ(included_step_1->tool_details->logo,
              "https://analytics.verse.bitcoin.com/logo.png");
    EXPECT_EQ(included_step_1->action->from_amount, "1000000");
    EXPECT_EQ(included_step_1->action->from_token->contract_address,
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48");
    EXPECT_EQ(included_step_1->action->from_token->chain_id, "0x1");
    EXPECT_EQ(included_step_1->action->from_token->symbol, "USDC");
    EXPECT_EQ(included_step_1->action->from_token->decimals, 6);
    EXPECT_EQ(included_step_1->action->from_token->name, "USD Coin");
    EXPECT_EQ(included_step_1->action->from_token->logo, "usdc.png");
    EXPECT_EQ(included_step_1->action->to_token->contract_address,
              "0xdAC17F958D2ee523a2206206994597C13D831ec7");
    EXPECT_EQ(included_step_1->action->to_token->chain_id, "0x1");
    EXPECT_EQ(included_step_1->action->to_token->symbol, "USDT");
    EXPECT_EQ(included_step_1->action->to_token->decimals, 6);
    EXPECT_EQ(included_step_1->action->to_token->name, "USDT");
    EXPECT_EQ(included_step_1->action->to_token->logo, "usdt.png");
    EXPECT_EQ(included_step_1->action->slippage, "0.005");
    EXPECT_EQ(included_step_1->estimate->tool, "verse-dex");
    EXPECT_EQ(included_step_1->estimate->from_amount, "1000000");
    EXPECT_EQ(included_step_1->estimate->to_amount, "1013654");
    EXPECT_EQ(included_step_1->estimate->to_amount_min, "1008586");
    EXPECT_EQ(included_step_1->estimate->approval_address,
              "0xB4B0ea46Fe0E9e8EAB4aFb765b527739F2718671");
    EXPECT_EQ(included_step_1->estimate->execution_duration, "30");
    ASSERT_EQ(included_step_1->estimate->fee_costs->size(), 1u);
    const auto& included_step_1_fee_cost =
        included_step_1->estimate->fee_costs->at(0);
    EXPECT_EQ(included_step_1_fee_cost->name, "LP Fee");
    EXPECT_EQ(included_step_1_fee_cost->description,
              "Fee paid to the Liquidity Provider");
    EXPECT_EQ(included_step_1_fee_cost->token->contract_address,
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48");
    EXPECT_EQ(included_step_1_fee_cost->token->chain_id, "0x1");
    EXPECT_EQ(included_step_1_fee_cost->token->symbol, "USDC");
    EXPECT_EQ(included_step_1_fee_cost->token->decimals, 6);
    EXPECT_EQ(included_step_1_fee_cost->token->name, "USD Coin");
    EXPECT_EQ(included_step_1_fee_cost->token->logo, "usdc.png");
    EXPECT_EQ(included_step_1_fee_cost->amount, "3000");
    EXPECT_EQ(included_step_1_fee_cost->percentage, "0.003");
    EXPECT_TRUE(included_step_1_fee_cost->included);
    ASSERT_EQ(included_step_1->estimate->gas_costs.size(), 1u);
    const auto& included_step_1_gas_cost =
        included_step_1->estimate->gas_costs.at(0);
    EXPECT_EQ(included_step_1_gas_cost->type, "SEND");
    EXPECT_EQ(included_step_1_gas_cost->estimate, "200000");
    EXPECT_EQ(included_step_1_gas_cost->limit, "260000");
    EXPECT_EQ(included_step_1_gas_cost->amount, "3524380397000000");
    EXPECT_EQ(included_step_1_gas_cost->token->contract_address, "");
    EXPECT_EQ(included_step_1_gas_cost->token->chain_id, "0x1");
    EXPECT_EQ(included_step_1_gas_cost->token->symbol, "ETH");
    EXPECT_EQ(included_step_1_gas_cost->token->decimals, 18);
    EXPECT_EQ(included_step_1_gas_cost->token->name, "ETH");
    EXPECT_EQ(included_step_1_gas_cost->token->logo, "eth.png");
    EXPECT_FALSE(included_step_1->included_steps);

    const auto& included_step_2 = step->included_steps->at(1);
    EXPECT_EQ(included_step_2->id, "b952ed38-1d5c-43bc-990a-468fd32d29b9");
    EXPECT_EQ(included_step_2->type, mojom::LiFiStepType::kCross);
    EXPECT_EQ(included_step_2->tool, "optimism");
    EXPECT_EQ(included_step_2->tool_details->key, "optimism");
    EXPECT_EQ(included_step_2->tool_details->name, "Optimism Gateway");
    EXPECT_EQ(included_step_2->tool_details->logo, "optimism.png");
    EXPECT_EQ(included_step_2->action->from_amount, "1008586");
    EXPECT_EQ(included_step_2->action->from_token->contract_address,
              "0xdAC17F958D2ee523a2206206994597C13D831ec7");
    EXPECT_EQ(included_step_2->action->from_token->chain_id, "0x1");
    EXPECT_EQ(included_step_2->action->from_token->symbol, "USDT");
    EXPECT_EQ(included_step_2->action->from_token->decimals, 6);
    EXPECT_EQ(included_step_2->action->from_token->name, "USDT");
    EXPECT_EQ(included_step_2->action->from_token->logo, "usdt.png");
    EXPECT_EQ(included_step_2->action->to_token->contract_address,
              "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58");
    EXPECT_EQ(included_step_2->action->to_token->chain_id, "0xa");
    EXPECT_EQ(included_step_2->action->to_token->symbol, "USDT");
    EXPECT_EQ(included_step_2->action->to_token->decimals, 6);
    EXPECT_EQ(included_step_2->action->to_token->name, "USDT");
    EXPECT_EQ(included_step_2->action->to_token->logo, "usdt.png");
    EXPECT_EQ(included_step_2->action->slippage, "0.005");
    EXPECT_EQ(included_step_2->action->destination_call_data, "0x0");
    EXPECT_EQ(included_step_2->estimate->tool, "optimism");
    EXPECT_EQ(included_step_2->estimate->from_amount, "1013654");
    EXPECT_EQ(included_step_2->estimate->to_amount, "1013654");
    EXPECT_EQ(included_step_2->estimate->to_amount_min, "1008586");
    EXPECT_EQ(included_step_2->estimate->approval_address,
              "0x99C9fc46f92E8a1c0deC1b1747d010903E884bE1");
    EXPECT_EQ(included_step_2->estimate->execution_duration, "76.944");
    EXPECT_TRUE(included_step_2->estimate->fee_costs->empty());
    ASSERT_EQ(included_step_2->estimate->gas_costs.size(), 1u);
    const auto& included_step_2_gas_cost =
        included_step_2->estimate->gas_costs.at(0);
    EXPECT_EQ(included_step_2_gas_cost->type, "SEND");
    EXPECT_EQ(included_step_2_gas_cost->estimate, "175000");
    EXPECT_EQ(included_step_2_gas_cost->limit, "227500");
    EXPECT_EQ(included_step_2_gas_cost->amount, "3083832847375000");
    EXPECT_EQ(included_step_2_gas_cost->token->contract_address, "");
    EXPECT_EQ(included_step_2_gas_cost->token->chain_id, "0x1");
    EXPECT_EQ(included_step_2_gas_cost->token->symbol, "ETH");
    EXPECT_EQ(included_step_2_gas_cost->token->decimals, 18);
    EXPECT_EQ(included_step_2_gas_cost->token->name, "ETH");
    EXPECT_EQ(included_step_2_gas_cost->token->logo, "eth.png");
    EXPECT_FALSE(included_step_2->included_steps);

    ASSERT_EQ(route->tags.size(), 3u);
    EXPECT_EQ(route->tags.at(0), "RECOMMENDED");
    EXPECT_EQ(route->tags.at(1), "CHEAPEST");
    EXPECT_EQ(route->tags.at(2), "FASTEST");

    EXPECT_EQ(route->unique_id, "optimism");
  }

  {
    // OK: USDC.e on Polygon --> SOL on Solana
    std::string json(R"(
      {
        "routes": [
          {
            "id": "4c901782-830f-454e-9ed8-6d246829799f",
            "fromChainId": "137",
            "fromAmountUSD": "20.00",
            "fromAmount": "20000000",
            "fromToken": {
              "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
              "chainId": "137",
              "symbol": "USDC.e",
              "decimals": "6",
              "name": "Bridged USD Coin",
              "coinKey": "USDCe",
              "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
              "priceUSD": "1"
            },
            "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
            "toChainId": "1151111081099710",
            "toAmountUSD": "17.15",
            "toAmount": "107802690",
            "toAmountMin": "104568610",
            "toToken": {
              "address": "11111111111111111111111111111111",
              "chainId": "1151111081099710",
              "symbol": "SOL",
              "decimals": "9",
              "name": "SOL",
              "coinKey": "SOL",
              "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
              "priceUSD": "159.11"
            },
            "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
            "gasCostUSD": "0.01",
            "containsSwitchChain": false,
            "steps": [
              {
                "type": "lifi",
                "id": "4c901782-830f-454e-9ed8-6d246829799f:0",
                "tool": "mayan",
                "toolDetails": {
                  "key": "mayan",
                  "name": "Mayan",
                  "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
                },
                "action": {
                  "fromToken": {
                    "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                    "chainId": "137",
                    "symbol": "USDC.e",
                    "decimals": "6",
                    "name": "Bridged USD Coin",
                    "coinKey": "USDCe",
                    "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                    "priceUSD": "1"
                  },
                  "fromAmount": "20000000",
                  "toToken": {
                    "address": "11111111111111111111111111111111",
                    "chainId": "1151111081099710",
                    "symbol": "SOL",
                    "decimals": "9",
                    "name": "SOL",
                    "coinKey": "SOL",
                    "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
                    "priceUSD": "159.11"
                  },
                  "fromChainId": "137",
                  "toChainId": "1151111081099710",
                  "slippage": "0.03",
                  "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                  "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4"
                },
                "estimate": {
                  "tool": "mayan",
                  "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                  "toAmountMin": "104568610",
                  "toAmount": "107802690",
                  "fromAmount": "20000000",
                  "feeCosts": [
                    {
                      "name": "Swap Relayer Fee",
                      "description": "Fee for the swap relayer",
                      "token": {
                        "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                        "chainId": "137",
                        "symbol": "USDC.e",
                        "decimals": "6",
                        "name": "Bridged USD Coin",
                        "coinKey": "USDCe",
                        "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                        "priceUSD": "1"
                      },
                      "amount": "2746612",
                      "amountUSD": "2.75",
                      "percentage": "0.1373305975",
                      "included": true
                    }
                  ],
                  "gasCosts": [
                    {
                      "type": "SEND",
                      "price": "43687550986",
                      "estimate": "370000",
                      "limit": "513000",
                      "amount": "16164393864820000",
                      "amountUSD": "0.01",
                      "token": {
                        "address": "0x0000000000000000000000000000000000000000",
                        "chainId": "137",
                        "symbol": "MATIC",
                        "decimals": "18",
                        "name": "MATIC",
                        "coinKey": "MATIC",
                        "logoURI": "https://static.debank.com/image/matic_token/logo_url/matic/6f5a6b6f0732a7a235131bd7804d357c.png",
                        "priceUSD": "0.4077"
                      }
                    }
                  ],
                  "executionDuration": "368",
                  "fromAmountUSD": "20.00",
                  "toAmountUSD": "17.15"
                },
                "includedSteps": [
                  {
                    "id": "e003be5c-5099-4f3a-8053-efb5767c4ba8",
                    "type": "cross",
                    "action": {
                      "fromChainId": "137",
                      "fromAmount": "20000000",
                      "fromToken": {
                        "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                        "chainId": "137",
                        "symbol": "USDC.e",
                        "decimals": "6",
                        "name": "Bridged USD Coin",
                        "coinKey": "USDCe",
                        "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                        "priceUSD": "1"
                      },
                      "toChainId": "1151111081099710",
                      "toToken": {
                        "address": "11111111111111111111111111111111",
                        "chainId": "1151111081099710",
                        "symbol": "SOL",
                        "decimals": "9",
                        "name": "SOL",
                        "coinKey": "SOL",
                        "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
                        "priceUSD": "159.11"
                      },
                      "slippage": "0.03",
                      "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                      "destinationGasConsumption": "0"
                    },
                    "estimate": {
                      "tool": "mayan",
                      "fromAmount": "20000000",
                      "toAmount": "107802690",
                      "toAmountMin": "104568610",
                      "gasCosts": [
                        {
                          "type": "SEND",
                          "price": "43687550986",
                          "estimate": "370000",
                          "limit": "555000",
                          "amount": "16164393864820000",
                          "amountUSD": "0.01",
                          "token": {
                            "address": "0x0000000000000000000000000000000000000000",
                            "chainId": "137",
                            "symbol": "MATIC",
                            "decimals": "18",
                            "name": "MATIC",
                            "coinKey": "MATIC",
                            "logoURI": "https://static.debank.com/image/matic_token/logo_url/matic/6f5a6b6f0732a7a235131bd7804d357c.png",
                            "priceUSD": "0.4077"
                          }
                        }
                      ],
                      "executionDuration": "368",
                      "approvalAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                      "feeCosts": [
                        {
                          "name": "Swap Relayer Fee",
                          "description": "Fee for the swap relayer",
                          "token": {
                            "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                            "chainId": "137",
                            "symbol": "USDC.e",
                            "decimals": "6",
                            "name": "Bridged USD Coin",
                            "coinKey": "USDCe",
                            "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                            "priceUSD": "1"
                          },
                          "amount": "2746612",
                          "amountUSD": "2.75",
                          "percentage": "0.1373305975",
                          "included": true
                        }
                      ]
                    },
                    "tool": "mayan",
                    "toolDetails": {
                      "key": "mayan",
                      "name": "Mayan",
                      "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
                    }
                  }
                ],
                "integrator": "lifi-api"
              }
            ],
            "tags": [
              "RECOMMENDED",
              "CHEAPEST",
              "FASTEST"
            ]
          }
        ],
        "unavailableRoutes": {
          "filteredOut": [],
          "failed": []
        }
      }
    )");

    auto quote = lifi::ParseQuoteResponse(ParseJson(json));
    ASSERT_TRUE(quote);

    ASSERT_EQ(quote->routes.size(), 1u);
    const auto& route = quote->routes.at(0);
    EXPECT_EQ(route->id, "4c901782-830f-454e-9ed8-6d246829799f");
    EXPECT_EQ(route->from_amount, "20000000");
    EXPECT_EQ(route->from_token->contract_address,
              "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174");
    EXPECT_EQ(route->from_token->chain_id, "0x89");
    EXPECT_EQ(route->from_token->symbol, "USDC.e");
    EXPECT_EQ(route->from_token->decimals, 6);
    EXPECT_EQ(route->from_token->name, "Bridged USD Coin");
    EXPECT_EQ(route->from_token->logo,
              "https://raw.githubusercontent.com/trustwallet/assets/master/"
              "blockchains/ethereum/assets/"
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png");
    EXPECT_EQ(route->from_address,
              "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0");
    EXPECT_EQ(route->to_amount, "107802690");
    EXPECT_EQ(route->to_amount_min, "104568610");
    EXPECT_EQ(route->to_token->contract_address, "");
    EXPECT_EQ(route->to_token->chain_id, "0x65");
    EXPECT_EQ(route->to_token->symbol, "SOL");
    EXPECT_EQ(route->to_token->decimals, 9);
    EXPECT_EQ(route->to_token->name, "SOL");
    EXPECT_EQ(route->to_token->logo,
              "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png");
    EXPECT_EQ(route->to_address, "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");

    ASSERT_EQ(route->steps.size(), 1u);
    const auto& step = route->steps.at(0);
    EXPECT_EQ(step->type, mojom::LiFiStepType::kLiFi);
    EXPECT_EQ(step->id, "4c901782-830f-454e-9ed8-6d246829799f:0");
    EXPECT_EQ(step->tool, "mayan");
    EXPECT_EQ(step->tool_details->key, "mayan");
    EXPECT_EQ(step->tool_details->name, "Mayan");
    EXPECT_EQ(step->tool_details->logo,
              "https://raw.githubusercontent.com/lifinance/types/main/src/"
              "assets/icons/bridges/mayan.png");
    EXPECT_EQ(step->action->from_token->contract_address,
              "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174");
    EXPECT_EQ(step->action->from_token->chain_id, "0x89");
    EXPECT_EQ(step->action->from_token->symbol, "USDC.e");
    EXPECT_EQ(step->action->from_token->decimals, 6);
    EXPECT_EQ(step->action->from_token->name, "Bridged USD Coin");
    EXPECT_EQ(step->action->from_token->logo,
              "https://raw.githubusercontent.com/trustwallet/assets/master/"
              "blockchains/ethereum/assets/"
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png");
    EXPECT_EQ(step->action->from_amount, "20000000");
    EXPECT_EQ(step->action->to_token->contract_address, "");
    EXPECT_EQ(step->action->to_token->chain_id, "0x65");
    EXPECT_EQ(step->action->to_token->symbol, "SOL");
    EXPECT_EQ(step->action->to_token->decimals, 9);
    EXPECT_EQ(step->action->to_token->name, "SOL");
    EXPECT_EQ(step->action->to_token->logo,
              "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png");
    EXPECT_EQ(step->action->slippage, "0.03");
    EXPECT_EQ(step->action->from_address,
              "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0");
    EXPECT_EQ(step->action->to_address,
              "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
    EXPECT_EQ(step->estimate->tool, "mayan");
    EXPECT_EQ(step->estimate->approval_address,
              "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE");
    EXPECT_EQ(step->estimate->to_amount_min, "104568610");
    EXPECT_EQ(step->estimate->to_amount, "107802690");
    EXPECT_EQ(step->estimate->from_amount, "20000000");
    ASSERT_EQ(step->estimate->fee_costs->size(), 1u);
    const auto& fee_cost = step->estimate->fee_costs->at(0);
    EXPECT_EQ(fee_cost->name, "Swap Relayer Fee");
    EXPECT_EQ(fee_cost->description, "Fee for the swap relayer");
    EXPECT_EQ(fee_cost->token->contract_address,
              "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174");
    EXPECT_EQ(fee_cost->token->chain_id, "0x89");
    EXPECT_EQ(fee_cost->token->symbol, "USDC.e");
    EXPECT_EQ(fee_cost->token->decimals, 6);
    EXPECT_EQ(fee_cost->token->name, "Bridged USD Coin");
    EXPECT_EQ(fee_cost->token->logo,
              "https://raw.githubusercontent.com/trustwallet/assets/master/"
              "blockchains/ethereum/assets/"
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png");
    EXPECT_EQ(fee_cost->amount, "2746612");
    EXPECT_EQ(fee_cost->percentage, "0.1373305975");
    EXPECT_TRUE(fee_cost->included);
    ASSERT_EQ(step->estimate->gas_costs.size(), 1u);
    const auto& gas_cost = step->estimate->gas_costs.at(0);
    EXPECT_EQ(gas_cost->type, "SEND");
    EXPECT_EQ(gas_cost->estimate, "370000");
    EXPECT_EQ(gas_cost->limit, "513000");
    EXPECT_EQ(gas_cost->amount, "16164393864820000");
    EXPECT_EQ(gas_cost->token->contract_address, "");
    EXPECT_EQ(gas_cost->token->chain_id, "0x89");
    EXPECT_EQ(gas_cost->token->symbol, "MATIC");
    EXPECT_EQ(gas_cost->token->decimals, 18);
    EXPECT_EQ(gas_cost->token->name, "MATIC");
    EXPECT_EQ(gas_cost->token->logo,
              "https://static.debank.com/image/matic_token/logo_url/matic/"
              "6f5a6b6f0732a7a235131bd7804d357c.png");
    EXPECT_EQ(step->estimate->execution_duration, "368");

    ASSERT_TRUE(step->included_steps);
    ASSERT_EQ(step->included_steps->size(), 1u);
    const auto& included_step = step->included_steps->at(0);
    EXPECT_EQ(included_step->id, "e003be5c-5099-4f3a-8053-efb5767c4ba8");
    EXPECT_EQ(included_step->type, mojom::LiFiStepType::kCross);
    EXPECT_EQ(included_step->tool, "mayan");
    EXPECT_EQ(included_step->tool_details->key, "mayan");
    EXPECT_EQ(included_step->tool_details->name, "Mayan");
    EXPECT_EQ(included_step->tool_details->logo,
              "https://raw.githubusercontent.com/lifinance/types/main/src/"
              "assets/icons/bridges/mayan.png");
    EXPECT_EQ(included_step->action->from_token->contract_address,
              "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174");
    EXPECT_EQ(included_step->action->from_token->chain_id, "0x89");
    EXPECT_EQ(included_step->action->from_token->symbol, "USDC.e");
    EXPECT_EQ(included_step->action->from_token->decimals, 6);
    EXPECT_EQ(included_step->action->from_token->name, "Bridged USD Coin");
    EXPECT_EQ(included_step->action->from_token->logo,
              "https://raw.githubusercontent.com/trustwallet/assets/master/"
              "blockchains/ethereum/assets/"
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png");
    EXPECT_EQ(included_step->action->from_amount, "20000000");
    EXPECT_EQ(included_step->action->to_token->contract_address, "");
    EXPECT_EQ(included_step->action->to_token->chain_id, "0x65");
    EXPECT_EQ(included_step->action->to_token->symbol, "SOL");
    EXPECT_EQ(included_step->action->to_token->decimals, 9);
    EXPECT_EQ(included_step->action->to_token->name, "SOL");
    EXPECT_EQ(included_step->action->to_token->logo,
              "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png");
    EXPECT_EQ(included_step->action->slippage, "0.03");
    EXPECT_EQ(included_step->action->from_address,
              "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0");
    EXPECT_EQ(included_step->estimate->tool, "mayan");
    EXPECT_EQ(included_step->estimate->from_amount, "20000000");
    EXPECT_EQ(included_step->estimate->to_amount, "107802690");
    EXPECT_EQ(included_step->estimate->to_amount_min, "104568610");
    ASSERT_EQ(included_step->estimate->gas_costs.size(), 1u);
    const auto& included_step_gas_cost =
        included_step->estimate->gas_costs.at(0);
    EXPECT_EQ(included_step_gas_cost->type, "SEND");
    EXPECT_EQ(included_step_gas_cost->estimate, "370000");
    EXPECT_EQ(included_step_gas_cost->limit, "555000");
    EXPECT_EQ(included_step_gas_cost->amount, "16164393864820000");
    EXPECT_EQ(included_step_gas_cost->token->contract_address, "");
    EXPECT_EQ(included_step_gas_cost->token->chain_id, "0x89");
    EXPECT_EQ(included_step_gas_cost->token->symbol, "MATIC");
    EXPECT_EQ(included_step_gas_cost->token->decimals, 18);
    EXPECT_EQ(included_step_gas_cost->token->name, "MATIC");
    EXPECT_EQ(included_step_gas_cost->token->logo,
              "https://static.debank.com/image/matic_token/logo_url/matic/"
              "6f5a6b6f0732a7a235131bd7804d357c.png");
    EXPECT_EQ(included_step->estimate->execution_duration, "368");
    EXPECT_EQ(included_step->estimate->approval_address,
              "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0");
    ASSERT_EQ(included_step->estimate->fee_costs->size(), 1u);
    const auto& included_step_fee_cost =
        included_step->estimate->fee_costs->at(0);
    EXPECT_EQ(included_step_fee_cost->name, "Swap Relayer Fee");
    EXPECT_EQ(included_step_fee_cost->description, "Fee for the swap relayer");
    EXPECT_EQ(included_step_fee_cost->token->contract_address,
              "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174");
    EXPECT_EQ(included_step_fee_cost->token->chain_id, "0x89");
    EXPECT_EQ(included_step_fee_cost->token->symbol, "USDC.e");
    EXPECT_EQ(included_step_fee_cost->token->decimals, 6);
    EXPECT_EQ(included_step_fee_cost->token->name, "Bridged USD Coin");
    EXPECT_EQ(included_step_fee_cost->token->logo,
              "https://raw.githubusercontent.com/trustwallet/assets/master/"
              "blockchains/ethereum/assets/"
              "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png");
    EXPECT_EQ(included_step_fee_cost->amount, "2746612");
    EXPECT_EQ(included_step_fee_cost->percentage, "0.1373305975");
    EXPECT_TRUE(included_step_fee_cost->included);

    ASSERT_EQ(route->tags.size(), 3u);
    EXPECT_EQ(route->tags.at(0), "RECOMMENDED");
    EXPECT_EQ(route->tags.at(1), "CHEAPEST");
    EXPECT_EQ(route->tags.at(2), "FASTEST");
  }
}

TEST(SwapResponseParserUnitTest, ParseLiFiTransactionResponse) {
  {
    // OK: valid Solana transaction
    std::string json(R"(
    {
      "transactionRequest": {
        "data": "b255Yg=="
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    ASSERT_TRUE(transaction);
    ASSERT_TRUE(transaction->is_solana_transaction());
    EXPECT_EQ(transaction->get_solana_transaction(), "b255Yg==");
  }

  {
    // KO: empty data field
    std::string json(R"(
    {
      "transactionRequest": {}
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing transactionRequest field
    std::string json(R"(
    {
      "foobar": 123
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // OK: valid EVM transaction
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    ASSERT_TRUE(transaction);
    ASSERT_TRUE(transaction->is_evm_transaction());
    const auto& evm_transaction = transaction->get_evm_transaction();
    EXPECT_EQ(evm_transaction->from,
              "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0");
    EXPECT_EQ(evm_transaction->to,
              "0x1111111254fb6c44bac0bed2854e76f90643097d");
    EXPECT_EQ(evm_transaction->chain_id, "0x64");
    EXPECT_EQ(evm_transaction->data, "0x...");
    EXPECT_EQ(evm_transaction->value, "0x0de0b6b3a7640000");
    EXPECT_EQ(evm_transaction->gas_price, "0xb2d05e00");
    EXPECT_EQ(evm_transaction->gas_limit, "0x0e9cb2");
  }

  {
    // KO: missing from field
    std::string json(R"(
    {
      "transactionRequest": {
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing to field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing chainId field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing data field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing value field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing gasPrice field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasLimit": "0x0e9cb2"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: missing gasLimit field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }

  {
    // KO: invalid chainId field
    std::string json(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "foobar",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00"
      }
    })");

    auto transaction = lifi::ParseTransactionResponse(ParseJson(json));
    EXPECT_FALSE(transaction);
  }
}

TEST(SwapResponseParserUnitTest, ParseLiFiErrorResponse) {
  std::string json(R"(
    {
      "message": "Invalid request"
    }
  )");

  auto error = lifi::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->message, "Invalid request");
}

TEST(SwapResponseParserUnitTest, ParseLiFiStatusResponse) {
  std::string json(R"(
    {
      "transactionId": "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016",
      "sending": {
        "txHash": "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "txLink": "https://optimistic.etherscan.io/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "amount": "2516860",
        "token": {
          "address": "0x7F5c764cBc14f9669B88837ca1490cCa17c31607",
          "chainId": "10",
          "symbol": "USDC.e",
          "decimals": "6",
          "name": "Bridged USD Coin",
          "coinKey": "USDCe",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
          "priceUSD": "1"
        },
        "chainId": "10",
        "gasPrice": "61761647",
        "gasUsed": "239193",
        "gasToken": {
          "address": "0x0000000000000000000000000000000000000000",
          "chainId": "10",
          "symbol": "ETH",
          "decimals": "18",
          "name": "ETH",
          "coinKey": "ETH",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2/logo.png",
          "priceUSD": "3399.92"
        },
        "gasAmount": "14772953630871",
        "gasAmountUSD": "0.05",
        "amountUSD": "2.52",
        "value": "0",
        "timestamp": "1721381005"
      },
      "receiving": {
        "txHash": "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "txLink": "https://optimistic.etherscan.io/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "amount": "741223046975948",
        "token": {
          "address": "0x0000000000000000000000000000000000000000",
          "chainId": "10",
          "symbol": "ETH",
          "decimals": "18",
          "name": "ETH",
          "coinKey": "ETH",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2/logo.png",
          "priceUSD": "3399.92"
        },
        "chainId": "10",
        "gasPrice": "61761647",
        "gasUsed": "239193",
        "gasToken": {
          "address": "0x0000000000000000000000000000000000000000",
          "chainId": "10",
          "symbol": "ETH",
          "decimals": "18",
          "name": "ETH",
          "coinKey": "ETH",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2/logo.png",
          "priceUSD": "3399.92"
        },
        "gasAmount": "14772953630871",
        "gasAmountUSD": "0.05",
        "amountUSD": "2.52",
        "value": "0",
        "timestamp": "1721381005"
      },
      "lifiExplorerLink": "https://explorer.li.fi/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
      "fromAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "toAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "tool": "paraswap",
      "status": "DONE",
      "substatus": "COMPLETED",
      "substatusMessage": "The transfer is complete.",
      "metadata": {
        "integrator": "brave"
      }
    }
  )");

  auto response = lifi::ParseStatusResponse(ParseJson(json));
  ASSERT_TRUE(response);

  EXPECT_EQ(
      response->transaction_id,
      "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016");
  EXPECT_EQ(response->sending->chain_id, "0xa");
  EXPECT_EQ(
      response->sending->tx_hash,
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(
      response->sending->tx_link,
      "https://optimistic.etherscan.io/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(response->sending->amount, "2516860");
  EXPECT_EQ(response->sending->contract_address,
            "0x7F5c764cBc14f9669B88837ca1490cCa17c31607");

  EXPECT_EQ(response->receiving->chain_id, "0xa");
  EXPECT_EQ(
      response->receiving->tx_hash,
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(
      response->receiving->tx_link,
      "https://optimistic.etherscan.io/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(response->receiving->amount, "741223046975948");
  EXPECT_EQ(response->receiving->contract_address,
            "0x0000000000000000000000000000000000000000");

  EXPECT_EQ(
      response->lifi_explorer_link,
      "https://explorer.li.fi/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(response->from_address,
            "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(response->to_address, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(response->tool, "paraswap");
  EXPECT_EQ(response->status, mojom::LiFiStatusCode::kDone);
  EXPECT_EQ(response->substatus, mojom::LiFiSubstatusCode::kCompleted);
  EXPECT_EQ(response->substatus_message, "The transfer is complete.");

  json = R"(
    {
      "transactionId": "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016",
      "sending": {
        "txHash": "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "txLink": "https://optimistic.etherscan.io/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "amount": "2516860",
        "token": {
          "address": "0x7F5c764cBc14f9669B88837ca1490cCa17c31607",
          "chainId": "10",
          "symbol": "USDC.e",
          "decimals": "6",
          "name": "Bridged USD Coin",
          "coinKey": "USDCe",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
          "priceUSD": "1"
        },
        "chainId": "10",
        "gasPrice": "61761647",
        "gasUsed": "239193",
        "gasToken": {
          "address": "0x0000000000000000000000000000000000000000",
          "chainId": "10",
          "symbol": "ETH",
          "decimals": "18",
          "name": "ETH",
          "coinKey": "ETH",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2/logo.png",
          "priceUSD": "3399.92"
        },
        "gasAmount": "14772953630871",
        "gasAmountUSD": "0.05",
        "amountUSD": "2.52",
        "value": "0",
        "timestamp": "1721381005"
      },
      "receiving": {
        "chainId": "10"
      },
      "lifiExplorerLink": "https://explorer.li.fi/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
      "fromAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "toAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "tool": "paraswap",
      "status": "PENDING",
      "substatus": "WAIT_DESTINATION_TRANSACTION",
      "substatusMessage": "The transfer is waiting for destination transaction.",
      "metadata": {
        "integrator": "brave"
      }
    }
  )";

  response = lifi::ParseStatusResponse(ParseJson(json));
  ASSERT_TRUE(response);

  EXPECT_EQ(
      response->transaction_id,
      "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016");
  EXPECT_EQ(response->sending->chain_id, "0xa");
  EXPECT_EQ(
      response->sending->tx_hash,
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(
      response->sending->tx_link,
      "https://optimistic.etherscan.io/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(response->sending->amount, "2516860");
  EXPECT_EQ(response->sending->contract_address,
            "0x7F5c764cBc14f9669B88837ca1490cCa17c31607");

  EXPECT_EQ(response->receiving->chain_id, "0xa");
  EXPECT_EQ(response->receiving->tx_hash, std::nullopt);
  EXPECT_EQ(response->receiving->tx_link, std::nullopt);
  EXPECT_EQ(response->receiving->amount, std::nullopt);
  EXPECT_EQ(response->receiving->contract_address, std::nullopt);

  EXPECT_EQ(
      response->lifi_explorer_link,
      "https://explorer.li.fi/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb");
  EXPECT_EQ(response->from_address,
            "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(response->to_address, "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4");
  EXPECT_EQ(response->tool, "paraswap");
  EXPECT_EQ(response->status, mojom::LiFiStatusCode::kPending);
  EXPECT_EQ(response->substatus,
            mojom::LiFiSubstatusCode::kWaitDestinationTransaction);
  EXPECT_EQ(response->substatus_message,
            "The transfer is waiting for destination transaction.");
}

TEST(SwapResponseParserUnitTest, ParseSquidErrorResponse) {
  std::string json(R"(
    {
      "message": "onChainQuoting must be a `boolean` type, but the final value was: `\"ani\"`.",
      "statusCode": "400",
      "type": "SCHEMA_VALIDATION_ERROR"
    }
  )");

  auto error = squid::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->message,
            "onChainQuoting must be a `boolean` type, but the "
            "final value was: `\"ani\"`.");
  EXPECT_EQ(error->type, mojom::SquidErrorType::kSchemaValidationError);

  json = R"(
    {
      "message": "The request is missing a required parameter.",
      "statusCode": "400",
      "type": "INVALID_REQUEST_ERROR"
    }
  )";

  error = squid::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->message, "The request is missing a required parameter.");
  EXPECT_EQ(error->type, mojom::SquidErrorType::kUnknownError);

  json = R"(
    {
      "statusCode": "400",
      "type": "BAD_REQUEST_ERROR",
      "message": "Unable to fetch token data"
    }
  )";

  error = squid::ParseErrorResponse(ParseJson(json));
  ASSERT_TRUE(error);
  EXPECT_EQ(error->type, mojom::SquidErrorType::kUnknownError);
  EXPECT_EQ(error->message, "Unable to fetch token data");
  EXPECT_TRUE(error->is_insufficient_liquidity);
}

TEST(SwapResponseParserUnitTest, ParseSquidQuoteResponse) {
  std::string json(R"(
    {
      "route": {
        "estimate": {
          "actions": [
            {
              "type": "wrap",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "42161",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": "2581.298038575404"
              },
              "toToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0x82af49447d8a07e3bd95bd0d56f35241523fbab1",
                "name": "Wrapped ETH",
                "symbol": "WETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/weth.svg",
                "coingeckoId": "weth",
                "axelarNetworkSymbol": "WETH",
                "subGraphIds": [
                  "arbitrum-weth-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "2583.905483882831"
              },
              "fromAmount": "10000000000000000",
              "toAmount": "10000000000000000",
              "toAmountMin": "10000000000000000",
              "exchangeRate": "1.0",
              "priceImpact": "0.00",
              "stage": "0",
              "provider": "Native Wrapper",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/weth.svg",
              "description": "Wrap ETH to WETH"
            },
            {
              "type": "swap",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "42161",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0x82af49447d8a07e3bd95bd0d56f35241523fbab1",
                "name": "Wrapped ETH",
                "symbol": "WETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/weth.svg",
                "coingeckoId": "weth",
                "axelarNetworkSymbol": "WETH",
                "subGraphIds": [
                  "arbitrum-weth-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "2583.905483882831"
              },
              "toToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xaf88d065e77c8cC2239327C5EDb3A432268e5831",
                "name": "USD Coin",
                "symbol": "USDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "usd-coin",
                "subGraphIds": [
                  "uusdc",
                  "cctp-uusdc-arbitrum-to-noble"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.999301671003392"
              },
              "fromAmount": "10000000000000000",
              "toAmount": "25826875",
              "toAmountMin": "25749394",
              "exchangeRate": "2582.6875",
              "priceImpact": "-0.0000627062536913",
              "stage": "0",
              "provider": "Uniswap V3",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/uniswap.svg",
              "description": "Swap from WETH to USDC"
            },
            {
              "type": "swap",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "42161",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xaf88d065e77c8cC2239327C5EDb3A432268e5831",
                "name": "USD Coin",
                "symbol": "USDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "usd-coin",
                "subGraphIds": [
                  "uusdc",
                  "cctp-uusdc-arbitrum-to-noble"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.999301671003392"
              },
              "toToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEB466342C4d449BC9f53A865D5Cb90586f405215",
                "name": "Axelar Wrapped USDC",
                "symbol": "axlUSDC",
                "axelarNetworkSymbol": "axlUSDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "axlusdc",
                "subGraphIds": [
                  "uusdc"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.9953463988901597"
              },
              "fromAmount": "25826875",
              "toAmount": "25825839",
              "toAmountMin": "25779352",
              "exchangeRate": "0.999959886745880018",
              "priceImpact": "-0.0000858142057914",
              "stage": "0",
              "provider": "Camelot V3",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/camelot.svg",
              "description": "Swap from USDC to axlUSDC"
            },
            {
              "type": "bridge",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "56",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEB466342C4d449BC9f53A865D5Cb90586f405215",
                "name": "Axelar Wrapped USDC",
                "symbol": "axlUSDC",
                "axelarNetworkSymbol": "axlUSDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "axlusdc",
                "subGraphIds": [
                  "uusdc"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.9953463988901597"
              },
              "toToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0x4268B8F0B87b6Eae5d897996E6b845ddbD99Adf3",
                "name": "Axelar Wrapped USDC",
                "symbol": "axlUSDC",
                "axelarNetworkSymbol": "axlUSDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "axlusdc",
                "subGraphIds": [
                  "uusdc"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.9953463988901597"
              },
              "fromAmount": "25825839",
              "toAmount": "25825839",
              "toAmountMin": "25825839",
              "exchangeRate": "1.0",
              "priceImpact": "0",
              "stage": "0",
              "provider": "Axelar",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/axelar.svg",
              "description": "Bridge axlUSDC to axlUSDC on BNB Chain",
              "estimatedDuration": "20"
            },
            {
              "type": "swap",
              "chainType": "evm",
              "fromChain": "56",
              "toChain": "56",
              "fromToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0x4268B8F0B87b6Eae5d897996E6b845ddbD99Adf3",
                "name": "Axelar Wrapped USDC",
                "symbol": "axlUSDC",
                "axelarNetworkSymbol": "axlUSDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "axlusdc",
                "subGraphIds": [
                  "uusdc"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.9953463988901597"
              },
              "toToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0x55d398326f99059fF775485246999027B3197955",
                "name": "Binance Pegged USDT",
                "symbol": "USDT",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdt.svg",
                "coingeckoId": "tether",
                "usdPrice": "0.9994421903530111"
              },
              "fromAmount": "25825839",
              "toAmount": "25813399115228268992",
              "toAmountMin": "25766934996820858107",
              "exchangeRate": "0.999518316335367419",
              "priceImpact": "-0.0000491970529387",
              "stage": "1",
              "provider": "Pancakeswap V3",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/pancakeswap.svg",
              "description": "Swap from axlUSDC to USDT"
            },
            {
              "type": "swap",
              "chainType": "evm",
              "fromChain": "56",
              "toChain": "56",
              "fromToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0x55d398326f99059fF775485246999027B3197955",
                "name": "Binance Pegged USDT",
                "symbol": "USDT",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdt.svg",
                "coingeckoId": "tether",
                "usdPrice": "0.9994421903530111"
              },
              "toToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0xbb4CdB9CBd36B01bD1cBaEBF2De08d9173bc095c",
                "name": "WBNB",
                "symbol": "WBNB",
                "decimals": "18",
                "axelarNetworkSymbol": "WBNB",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
                "coingeckoId": "binancecoin",
                "subGraphIds": [
                  "wbnb-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "515.821925157587"
              },
              "fromAmount": "25813399115228268992",
              "toAmount": "49836297930093733",
              "toAmountMin": "49686789036303451",
              "exchangeRate": "0.001930636787027922",
              "priceImpact": "0.0003558720202861",
              "stage": 1,
              "provider": "Pancakeswap V2",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/pancakeswap.svg",
              "description": "Swap from USDT to WBNB"
            },
            {
              "type": "wrap",
              "chainType": "evm",
              "fromChain": "56",
              "toChain": "56",
              "fromToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0xbb4CdB9CBd36B01bD1cBaEBF2De08d9173bc095c",
                "name": "WBNB",
                "symbol": "WBNB",
                "decimals": "18",
                "axelarNetworkSymbol": "WBNB",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
                "coingeckoId": "binancecoin",
                "subGraphIds": [
                  "wbnb-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "515.821925157587"
              },
              "toToken": {
                "type": "evm",
                "chainId": "56",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "BNB",
                "symbol": "BNB",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
                "coingeckoId": "binancecoin",
                "axelarNetworkSymbol": "WBNB",
                "subGraphIds": [
                  "wbnb-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "515.821925157587"
              },
              "fromAmount": "49836297930093733",
              "toAmount": "49836297930093733",
              "toAmountMin": "49686789036303451",
              "exchangeRate": "1.0",
              "priceImpact": "0.00",
              "stage": "1",
              "provider": "Native Wrapper",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/weth.svg",
              "description": "Unwrap WBNB to BNB"
            }
          ],
          "fromAmount": "10000000000000000",
          "toAmount": "49836297930093733",
          "toAmountMin": "49686789036303451",
          "sendAmount": "10000000000000000",
          "exchangeRate": "4.9836297930093733",
          "aggregatePriceImpact": "0.0",
          "fromAmountUSD": "25.81",
          "toAmountUSD": "25.70",
          "toAmountMinUSD": "25.62",
          "aggregateSlippage": "0.9600000000000001",
          "index": "0",
          "fromToken": {
            "type": "evm",
            "chainId": "42161",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "Ethereum",
            "symbol": "ETH",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
            "coingeckoId": "ethereum",
            "usdPrice": "2581.298038575404"
          },
          "toToken": {
            "type": "evm",
            "chainId": "56",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "BNB",
            "symbol": "BNB",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
            "coingeckoId": "binancecoin",
            "axelarNetworkSymbol": "WBNB",
            "subGraphIds": [
              "wbnb-wei"
            ],
            "subGraphOnly": false,
            "usdPrice": "515.821925157587"
          },
          "isBoostSupported": true,
          "feeCosts": [
            {
              "amount": "311053437062551",
              "amountUsd": "0.80",
              "description": "Gas receiver fee",
              "gasLimit": "696400",
              "gasMultiplier": "1.1550000000000002",
              "name": "Gas receiver fee",
              "token": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": 2581.298038575404
              }
            },
            {
              "amount": "39742490455932",
              "amountUsd": "0.10",
              "description": "Boost fee for Arbitrum to BNB Chain",
              "name": "Boost fee",
              "token": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": 2581.298038575404
              }
            }
          ],
          "gasCosts": [
            {
              "type": "executeCall",
              "token": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": 2581.298038575404
              },
              "amount": "1452012968376000",
              "gasLimit": "953658",
              "amountUsd": "3.75"
            }
          ],
          "estimatedRouteDuration": "20"
        },
        "transactionRequest": {
          "routeType": "CALL_BRIDGE_CALL",
          "target": "0xce16F69375520ab01377ce7B88f5BA8C48F8D666",
          "data": "0xdeadbeef",
          "value": "1000368231439378717",
          "gasLimit": "995464",
          "lastBaseFeePerGas": "10000000",
          "maxFeePerGas": "1520000000",
          "maxPriorityFeePerGas": "1500000000",
          "gasPrice": "10000000",
          "requestId": "c8f8eb102224d0de969ce595612ef1ab"
        }
      }
    }
  )");

  auto response = squid::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(response);

  ASSERT_EQ(response->actions.size(), 7u);
  EXPECT_EQ(response->actions[0]->type, mojom::SquidActionType::kWrap);
  EXPECT_EQ(response->actions[0]->description, "Wrap ETH to WETH");
  EXPECT_EQ(response->actions[0]->provider, "Native Wrapper");
  EXPECT_EQ(response->actions[0]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/weth.svg");
  EXPECT_EQ(response->actions[0]->from_amount, "10000000000000000");
  EXPECT_EQ(response->actions[0]->from_token->contract_address, "");
  EXPECT_EQ(response->actions[0]->from_token->name, "Ethereum");
  EXPECT_EQ(response->actions[0]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/eth.svg");
  EXPECT_EQ(response->actions[0]->from_token->symbol, "ETH");
  EXPECT_EQ(response->actions[0]->from_token->decimals, 18);
  EXPECT_EQ(response->actions[0]->from_token->coingecko_id, "ethereum");
  EXPECT_EQ(response->actions[0]->from_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[0]->from_token->coin, mojom::CoinType::ETH);
  EXPECT_EQ(response->actions[0]->to_amount, "10000000000000000");
  EXPECT_EQ(response->actions[0]->to_amount_min, "10000000000000000");
  EXPECT_EQ(response->actions[0]->to_token->contract_address,
            "0x82af49447d8a07e3bd95bd0d56f35241523fbab1");
  EXPECT_EQ(response->actions[0]->to_token->name, "Wrapped ETH");
  EXPECT_EQ(response->actions[0]->to_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/weth.svg");
  EXPECT_EQ(response->actions[0]->to_token->symbol, "WETH");
  EXPECT_EQ(response->actions[0]->to_token->decimals, 18);
  EXPECT_EQ(response->actions[0]->to_token->coingecko_id, "weth");
  EXPECT_EQ(response->actions[0]->to_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[0]->to_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[1]->type, mojom::SquidActionType::kSwap);
  EXPECT_EQ(response->actions[1]->description, "Swap from WETH to USDC");
  EXPECT_EQ(response->actions[1]->provider, "Uniswap V3");
  EXPECT_EQ(response->actions[1]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/uniswap.svg");
  EXPECT_EQ(response->actions[1]->from_amount, "10000000000000000");
  EXPECT_EQ(response->actions[1]->from_token->contract_address,
            "0x82af49447d8a07e3bd95bd0d56f35241523fbab1");
  EXPECT_EQ(response->actions[1]->from_token->name, "Wrapped ETH");
  EXPECT_EQ(response->actions[1]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/weth.svg");
  EXPECT_EQ(response->actions[1]->from_token->symbol, "WETH");
  EXPECT_EQ(response->actions[1]->from_token->decimals, 18);
  EXPECT_EQ(response->actions[1]->from_token->coingecko_id, "weth");
  EXPECT_EQ(response->actions[1]->from_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[1]->from_token->coin, mojom::CoinType::ETH);
  EXPECT_EQ(response->actions[1]->to_amount, "25826875");
  EXPECT_EQ(response->actions[1]->to_amount_min, "25749394");
  EXPECT_EQ(response->actions[1]->to_token->contract_address,
            "0xaf88d065e77c8cC2239327C5EDb3A432268e5831");
  EXPECT_EQ(response->actions[1]->to_token->name, "USD Coin");
  EXPECT_EQ(response->actions[1]->to_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/usdc.svg");
  EXPECT_EQ(response->actions[1]->to_token->symbol, "USDC");
  EXPECT_EQ(response->actions[1]->to_token->decimals, 6);
  EXPECT_EQ(response->actions[1]->to_token->coingecko_id, "usd-coin");
  EXPECT_EQ(response->actions[1]->to_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[1]->to_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[2]->type, mojom::SquidActionType::kSwap);
  EXPECT_EQ(response->actions[2]->description, "Swap from USDC to axlUSDC");
  EXPECT_EQ(response->actions[2]->provider, "Camelot V3");
  EXPECT_EQ(response->actions[2]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/camelot.svg");
  EXPECT_EQ(response->actions[2]->from_amount, "25826875");
  EXPECT_EQ(response->actions[2]->from_token->contract_address,
            "0xaf88d065e77c8cC2239327C5EDb3A432268e5831");
  EXPECT_EQ(response->actions[2]->from_token->name, "USD Coin");
  EXPECT_EQ(response->actions[2]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/usdc.svg");
  EXPECT_EQ(response->actions[2]->from_token->symbol, "USDC");
  EXPECT_EQ(response->actions[2]->from_token->decimals, 6);
  EXPECT_EQ(response->actions[2]->from_token->coingecko_id, "usd-coin");
  EXPECT_EQ(response->actions[2]->from_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[2]->from_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[3]->type, mojom::SquidActionType::kBridge);
  EXPECT_EQ(response->actions[3]->description,
            "Bridge axlUSDC to axlUSDC on BNB Chain");
  EXPECT_EQ(response->actions[3]->provider, "Axelar");
  EXPECT_EQ(response->actions[3]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/axelar.svg");
  EXPECT_EQ(response->actions[3]->from_amount, "25825839");
  EXPECT_EQ(response->actions[3]->from_token->contract_address,
            "0xEB466342C4d449BC9f53A865D5Cb90586f405215");
  EXPECT_EQ(response->actions[3]->from_token->name, "Axelar Wrapped USDC");
  EXPECT_EQ(response->actions[3]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/usdc.svg");
  EXPECT_EQ(response->actions[3]->from_token->symbol, "axlUSDC");
  EXPECT_EQ(response->actions[3]->from_token->decimals, 6);
  EXPECT_EQ(response->actions[3]->from_token->coingecko_id, "axlusdc");
  EXPECT_EQ(response->actions[3]->from_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->actions[3]->from_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[4]->type, mojom::SquidActionType::kSwap);
  EXPECT_EQ(response->actions[4]->description, "Swap from axlUSDC to USDT");
  EXPECT_EQ(response->actions[4]->provider, "Pancakeswap V3");
  EXPECT_EQ(response->actions[4]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/pancakeswap.svg");
  EXPECT_EQ(response->actions[4]->from_amount, "25825839");
  EXPECT_EQ(response->actions[4]->from_token->contract_address,
            "0x4268B8F0B87b6Eae5d897996E6b845ddbD99Adf3");
  EXPECT_EQ(response->actions[4]->from_token->name, "Axelar Wrapped USDC");
  EXPECT_EQ(response->actions[4]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/usdc.svg");
  EXPECT_EQ(response->actions[4]->from_token->symbol, "axlUSDC");
  EXPECT_EQ(response->actions[4]->from_token->decimals, 6);
  EXPECT_EQ(response->actions[4]->from_token->coingecko_id, "axlusdc");
  EXPECT_EQ(response->actions[4]->from_token->chain_id, "0x38");
  EXPECT_EQ(response->actions[4]->from_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[5]->type, mojom::SquidActionType::kSwap);
  EXPECT_EQ(response->actions[5]->description, "Swap from USDT to WBNB");
  EXPECT_EQ(response->actions[5]->provider, "Pancakeswap V2");
  EXPECT_EQ(response->actions[5]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/pancakeswap.svg");
  EXPECT_EQ(response->actions[5]->from_amount, "25813399115228268992");
  EXPECT_EQ(response->actions[5]->from_token->contract_address,
            "0x55d398326f99059fF775485246999027B3197955");
  EXPECT_EQ(response->actions[5]->from_token->name, "Binance Pegged USDT");
  EXPECT_EQ(response->actions[5]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/usdt.svg");
  EXPECT_EQ(response->actions[5]->from_token->symbol, "USDT");
  EXPECT_EQ(response->actions[5]->from_token->decimals, 18);
  EXPECT_EQ(response->actions[5]->from_token->coingecko_id, "tether");
  EXPECT_EQ(response->actions[5]->from_token->chain_id, "0x38");
  EXPECT_EQ(response->actions[5]->from_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->actions[6]->type, mojom::SquidActionType::kWrap);
  EXPECT_EQ(response->actions[6]->description, "Unwrap WBNB to BNB");
  EXPECT_EQ(response->actions[6]->provider, "Native Wrapper");
  EXPECT_EQ(response->actions[6]->logo_uri,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "providers/weth.svg");
  EXPECT_EQ(response->actions[6]->from_amount, "49836297930093733");
  EXPECT_EQ(response->actions[6]->from_token->contract_address,
            "0xbb4CdB9CBd36B01bD1cBaEBF2De08d9173bc095c");
  EXPECT_EQ(response->actions[6]->from_token->name, "WBNB");
  EXPECT_EQ(response->actions[6]->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/bnb.svg");
  EXPECT_EQ(response->actions[6]->from_token->symbol, "WBNB");
  EXPECT_EQ(response->actions[6]->from_token->decimals, 18);
  EXPECT_EQ(response->actions[6]->from_token->coingecko_id, "binancecoin");
  EXPECT_EQ(response->actions[6]->from_token->chain_id, "0x38");
  EXPECT_EQ(response->actions[6]->from_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->aggregate_price_impact, "0.0");
  EXPECT_EQ(response->aggregate_slippage, "0.9600000000000001");
  EXPECT_EQ(response->estimated_route_duration, "20");
  EXPECT_EQ(response->exchange_rate, "4.9836297930093733");

  ASSERT_EQ(response->fee_costs.size(), 2u);
  EXPECT_EQ(response->fee_costs[0]->amount, "311053437062551");
  EXPECT_EQ(response->fee_costs[0]->description, "Gas receiver fee");
  EXPECT_EQ(response->fee_costs[0]->name, "Gas receiver fee");
  EXPECT_EQ(response->fee_costs[0]->token->contract_address, "");
  EXPECT_EQ(response->fee_costs[0]->token->name, "Ethereum");
  EXPECT_EQ(response->fee_costs[0]->token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/eth.svg");
  EXPECT_EQ(response->fee_costs[0]->token->symbol, "ETH");
  EXPECT_EQ(response->fee_costs[0]->token->decimals, 18);
  EXPECT_EQ(response->fee_costs[0]->token->coingecko_id, "ethereum");
  EXPECT_EQ(response->fee_costs[0]->token->chain_id, "0xa4b1");
  EXPECT_EQ(response->fee_costs[0]->token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->fee_costs[1]->amount, "39742490455932");
  EXPECT_EQ(response->fee_costs[1]->description,
            "Boost fee for Arbitrum to "
            "BNB Chain");
  EXPECT_EQ(response->fee_costs[1]->name, "Boost fee");
  EXPECT_EQ(response->fee_costs[1]->token->contract_address, "");
  EXPECT_EQ(response->fee_costs[1]->token->name, "Ethereum");
  EXPECT_EQ(response->fee_costs[1]->token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/eth.svg");
  EXPECT_EQ(response->fee_costs[1]->token->symbol, "ETH");
  EXPECT_EQ(response->fee_costs[1]->token->decimals, 18);
  EXPECT_EQ(response->fee_costs[1]->token->coingecko_id, "ethereum");
  EXPECT_EQ(response->fee_costs[1]->token->chain_id, "0xa4b1");
  EXPECT_EQ(response->fee_costs[1]->token->coin, mojom::CoinType::ETH);

  ASSERT_EQ(response->gas_costs.size(), 1u);
  EXPECT_EQ(response->gas_costs[0]->amount, "1452012968376000");
  EXPECT_EQ(response->gas_costs[0]->gas_limit, "953658");
  EXPECT_EQ(response->gas_costs[0]->token->contract_address, "");
  EXPECT_EQ(response->gas_costs[0]->token->name, "Ethereum");
  EXPECT_EQ(response->gas_costs[0]->token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/eth.svg");
  EXPECT_EQ(response->gas_costs[0]->token->symbol, "ETH");
  EXPECT_EQ(response->gas_costs[0]->token->decimals, 18);
  EXPECT_EQ(response->gas_costs[0]->token->coingecko_id, "ethereum");
  EXPECT_EQ(response->gas_costs[0]->token->chain_id, "0xa4b1");
  EXPECT_EQ(response->gas_costs[0]->token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->is_boost_supported, true);
  EXPECT_EQ(response->from_amount, "10000000000000000");
  EXPECT_EQ(response->from_token->contract_address, "");
  EXPECT_EQ(response->from_token->name, "Ethereum");
  EXPECT_EQ(response->from_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/eth.svg");
  EXPECT_EQ(response->from_token->symbol, "ETH");
  EXPECT_EQ(response->from_token->decimals, 18);
  EXPECT_EQ(response->from_token->coingecko_id, "ethereum");
  EXPECT_EQ(response->from_token->chain_id, "0xa4b1");
  EXPECT_EQ(response->from_token->coin, mojom::CoinType::ETH);
  EXPECT_EQ(response->to_amount, "49836297930093733");
  EXPECT_EQ(response->to_token->contract_address, "");
  EXPECT_EQ(response->to_token->name, "BNB");
  EXPECT_EQ(response->to_token->logo,
            "https://raw.githubusercontent.com/0xsquid/assets/main/images/"
            "tokens/bnb.svg");
  EXPECT_EQ(response->to_token->symbol, "BNB");
  EXPECT_EQ(response->to_token->decimals, 18);
  EXPECT_EQ(response->to_token->coingecko_id, "binancecoin");
  EXPECT_EQ(response->to_token->chain_id, "0x38");
  EXPECT_EQ(response->to_token->coin, mojom::CoinType::ETH);

  EXPECT_EQ(response->allowance_target,
            "0xce16F69375520ab01377ce7B88f5BA8C48F8D666");
}

TEST(SwapResponseParserUnitTest, DebugSquid) {
  std::string json(R"(
  {
    "route": {
        "estimate": {
            "actions": [],
            "aggregatePriceImpact": "0.0",
            "aggregateSlippage": "0.5",
            "estimatedRouteDuration": "10",
            "exchangeRate": "0.3718245",
            "feeCosts": [],
            "fromAmount": "2000000000000000000",
            "fromAmountUSD": "0.73",
            "fromToken": {
                "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                "axelarNetworkSymbol": "MATIC",
                "chainId": "137",
                "coingeckoId": "matic-network",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/matic.svg",
                "name": "POL",
                "subGraphIds": [
                    "wmatic-wei"
                ],
                "subGraphOnly": false,
                "symbol": "POL",
                "type": "evm",
                "usdPrice": "0.36997782427450376",
                "volatility": "2"
            },
            "gasCosts": [],
            "isBoostSupported": false,
            "toAmount": "743649",
            "toAmountMin": "739930",
            "toAmountMinUSD": "0.73",
            "toAmountUSD": "0.74",
            "toToken": {
                "address": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
                "axelarNetworkSymbol": "USDC",
                "chainId": "137",
                "coingeckoId": "usd-coin",
                "decimals": "6",
                "interchainTokenId": null,
                "logoURI": "https://raw.githubusercontent.com/axelarnetwork/axelar-configs/main/images/tokens/usdc.svg",
                "name": "Polygon USDC",
                "subGraphIds": [
                    "polygon-uusdc"
                ],
                "subGraphOnly": true,
                "symbol": "USDC",
                "type": "evm",
                "usdPrice": "0.997429327281814",
                "volatility": "0"
            }
        },
        "transactionRequest": {
          "data": "0x58181a80000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0000000000000000000000000000000000000000000000001bc16d674ec8000000000000000000000000000000000000000000000000000000000000000000600000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000001a0000000000000000000000000000000000000000000000000000000000000032000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf12700000000000000000000000000000000000000000000000001bc16d674ec8000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000e00000000000000000000000000000000000000000000000000000000000000004d0e30db00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf1270000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf1270000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000044095ea7b300000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf12700000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000100000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001c000000000000000000000000000000000000000000000000000000000000000e404e45aaf0000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf12700000000000000000000000002791bca1f2de4661ed88a30c99a7a9449aa8417400000000000000000000000000000000000000000000000000000000000001f4000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba40000000000000000000000000000000000000000000000001bc16d674ec8000000000000000000000000000000000000000000000000000000000000000b4a5a00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000d500b1d8e8ef31e21c99d1db9a6444d3adf12700000000000000000000000000000000000000000000000000000000000000004042d96bd785965030498e8f18dbcd5e2",
          "gasLimit": "363500",
          "gasPrice": "30000000125",
          "lastBaseFeePerGas": "125",
          "maxFeePerGas": "1500000250",
          "maxPriorityFeePerGas": "1500000000",
          "routeType": "EVM_ONLY",
          "target": "0xce16F69375520ab01377ce7B88f5BA8C48F8D666",
          "value": "2000000000000000000"
        }
    }
}
  )");

  auto response = squid::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(response);
}

TEST(SwapResponseParserUnitTest, ParseSquidTransactionResponse) {
  std::string json(R"(
    {
      "route": {
        "estimate": {
          "actions": [],
          "fromAmount": "10000000000000000",
          "toAmount": "49836297930093733",
          "toAmountMin": "49686789036303451",
          "sendAmount": "10000000000000000",
          "exchangeRate": "4.9836297930093733",
          "aggregatePriceImpact": "0.0",
          "fromAmountUSD": "25.81",
          "toAmountUSD": "25.70",
          "toAmountMinUSD": "25.62",
          "aggregateSlippage": "0.9600000000000001",
          "index": "0",
          "fromToken": {
            "type": "evm",
            "chainId": "42161",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "Ethereum",
            "symbol": "ETH",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
            "coingeckoId": "ethereum",
            "usdPrice": "2581.298038575404"
          },
          "toToken": {
            "type": "evm",
            "chainId": "56",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "BNB",
            "symbol": "BNB",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
            "coingeckoId": "binancecoin",
            "axelarNetworkSymbol": "WBNB",
            "subGraphIds": [
              "wbnb-wei"
            ],
            "subGraphOnly": false,
            "usdPrice": "515.821925157587"
          },
          "isBoostSupported": true,
          "feeCosts": [],
          "gasCosts": [],
          "estimatedRouteDuration": "20"
        },
        "transactionRequest": {
          "routeType": "CALL_BRIDGE_CALL",
          "target": "0xce16F69375520ab01377ce7B88f5BA8C48F8D666",
          "data": "0x846a1bc6000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0000000000000000000000000000000000000000000000000de0b6b3a764000000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000a600000000000000000000000000000000000000000000000000000000000000aa00000000000000000000000000000000000000000000000000000000000000ae00000000000000000000000000000000000000000000000000000000000000b40000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba40000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000500000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001e0000000000000000000000000000000000000000000000000000000000000036000000000000000000000000000000000000000000000000000000000000005800000000000000000000000000000000000000000000000000000000000000700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab10000000000000000000000000000000000000000000000000de0b6b3a764000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000e00000000000000000000000000000000000000000000000000000000000000004d0e30db000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000044095ea7b300000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab10000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000100000000000000000000000068b3465833fb72a70ecdf485e0e4c7bd8665fc45000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001c000000000000000000000000000000000000000000000000000000000000000e404e45aaf00000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab1000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e583100000000000000000000000000000000000000000000000000000000000001f4000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c0000000000000000000000000000000000000000000000000de0b6b3a764000000000000000000000000000000000000000000000000000000000000957f6f55000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000082af49447d8a07e3bd95bd0d56f35241523fbab100000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e5831000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000044095ea7b300000000000000000000000032226588378236fd0c7c4053999f88ac0e5cac77ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e58310000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000100000000000000000000000032226588378236fd0c7c4053999f88ac0e5cac77000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001c000000000000000000000000000000000000000000000000000000000000000e404e45aaf000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e5831000000000000000000000000eb466342c4d449bc9f53a865d5cb90586f4052150000000000000000000000000000000000000000000000000000000000000064000000000000000000000000ce16f69375520ab01377ce7b88f5ba8c48f8d6660000000000000000000000000000000000000000000000000000000095f2983d0000000000000000000000000000000000000000000000000000000095a775a50000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000af88d065e77c8cc2239327c5edb3a432268e58310000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000761786c5553444300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000762696e616e636500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002a307863653136463639333735353230616230313337376365374238386635424138433438463844363636000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000c100000000000000000000000000000000000000000000000000000000000000040000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4000000000000000000000000000000000000000000000000000000000000000700000000000000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000000000000000000001e000000000000000000000000000000000000000000000000000000000000003600000000000000000000000000000000000000000000000000000000000000580000000000000000000000000000000000000000000000000000000000000070000000000000000000000000000000000000000000000000000000000000009200000000000000000000000000000000000000000000000000000000000000a8000000000000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000004268b8f0b87b6eae5d897996e6b845ddbd99adf300000000000000000000000000000000000000000000000000000000000000000000000000000000000000004268b8f0b87b6eae5d897996e6b845ddbd99adf3000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000044095ea7b300000000000000000000000013f4ea83d0bd40e75c8222255bc855a974568dd4ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000004268b8f0b87b6eae5d897996e6b845ddbd99adf30000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000100000000000000000000000013f4ea83d0bd40e75c8222255bc855a974568dd4000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001c000000000000000000000000000000000000000000000000000000000000000e404e45aaf0000000000000000000000004268b8f0b87b6eae5d897996e6b845ddbd99adf300000000000000000000000055d398326f99059ff775485246999027b31979550000000000000000000000000000000000000000000000000000000000000064000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c0000000000000000000000000000000000000000000000000000000095ec8b670000000000000000000000000000000000000000000000881849beaa6df927fb00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000004268b8f0b87b6eae5d897996e6b845ddbd99adf30000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000055d398326f99059ff775485246999027b3197955000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000044095ea7b300000000000000000000000013f4ea83d0bd40e75c8222255bc855a974568dd4ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000055d398326f99059ff775485246999027b31979550000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000100000000000000000000000013f4ea83d0bd40e75c8222255bc855a974568dd4000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000001c000000000000000000000000000000000000000000000000000000000000000e404e45aaf00000000000000000000000055d398326f99059ff775485246999027b3197955000000000000000000000000bb4cdb9cbd36b01bd1cbaebf2de08d9173bc095c00000000000000000000000000000000000000000000000000000000000001f4000000000000000000000000ea749fd6ba492dbc14c24fe8a3d08769229b896c000000000000000000000000000000000000000000000088571d197d3d9905a300000000000000000000000000000000000000000000000042b9a634a68705a3000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000055d398326f99059ff775485246999027b319795500000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000001000000000000000000000000bb4cdb9cbd36b01bd1cbaebf2de08d9173bc095c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000000242e1a7d4d0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000bb4cdb9cbd36b01bd1cbaebf2de08d9173bc095c00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000000000a92d461a9a988a7f11ec285d39783a637fdd6ba4000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0000000000000000000000000000000000000000000000000000000000000000c8f8eb102224d0de969ce595612ef1ab00000000000000000000000000000000c8f8eb102224d0de969ce595612ef1ab",
          "value": "1000368231439378717",
          "gasLimit": "995464",
          "lastBaseFeePerGas": "10000000",
          "maxFeePerGas": "1520000000",
          "maxPriorityFeePerGas": "1500000000",
          "gasPrice": "10000000",
          "requestId": "c8f8eb102224d0de969ce595612ef1ab"
        }
      }
    }
  )");

  auto response = squid::ParseTransactionResponse(ParseJson(json));
  ASSERT_TRUE(response);
}

}  // namespace brave_wallet
