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

const char* GetJupiterQuoteTemplate() {
  return R"(
    {
      "data": [
        {
          "inAmount": "%s",
          "outAmount": "261273",
          "amount": "10000",
          "otherAmountThreshold": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": "0.008955716118219659",
          "slippageBps": "50",
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": "10000",
              "outAmount": "117001203",
              "priceImpactPct": "0.0000001196568750220778",
              "lpFee": {
                "amount": "%s",
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": "0.003"
              },
              "platformFee": {
                "amount": "0",
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": "0"
              }
            }
          ]
        }
      ],
      "timeTaken": "0.044471802000089156"
    })";
}

const char* GetJupiterQuoteTemplateForPriceImpact() {
  return R"(
    {
      "data": [
        {
          "inAmount": "10000",
          "outAmount": "261273",
          "amount": "10000",
          "otherAmountThreshold": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": %s,
          "slippageBps": "50",
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": "10000",
              "outAmount": "117001203",
              "priceImpactPct": %s,
              "lpFee": {
                "amount": "10000",
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": "0.003"
              },
              "platformFee": {
                "amount": "0",
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": "0"
              }
            }
          ]
        }
      ],
      "timeTaken": "0.044471802000089156"
    })";
}
}  // namespace

TEST(SwapResponseParserUnitTest, ParsePriceQuote) {
  // Case 1: non-null zeroExFee
  std::string json(R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee": {
          "feeType": "volume",
          "feeToken": "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063",
          "feeAmount": "148470027512868522",
          "billingType": "on-chain"
        }
      }
    }
  )");
  mojom::SwapResponsePtr swap_response =
      ParseSwapResponse(ParseJson(json), false);
  ASSERT_TRUE(swap_response);

  EXPECT_EQ(swap_response->price, "1916.27547998814058355");
  EXPECT_TRUE(swap_response->guaranteed_price.empty());
  EXPECT_TRUE(swap_response->to.empty());
  EXPECT_TRUE(swap_response->data.empty());

  EXPECT_EQ(swap_response->value, "0");
  EXPECT_EQ(swap_response->gas, "719000");
  EXPECT_EQ(swap_response->estimated_gas, "719001");
  EXPECT_EQ(swap_response->gas_price, "26000000000");
  EXPECT_EQ(swap_response->protocol_fee, "0");
  EXPECT_EQ(swap_response->minimum_protocol_fee, "0");
  EXPECT_EQ(swap_response->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(swap_response->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(swap_response->buy_amount, "1000000000000000000000");
  EXPECT_EQ(swap_response->sell_amount, "1916275479988140583549706");
  EXPECT_EQ(swap_response->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(swap_response->sell_token_to_eth_rate, "1900.44962824532464391");
  EXPECT_EQ(swap_response->buy_token_to_eth_rate, "1");
  EXPECT_EQ(swap_response->estimated_price_impact, "0.7232");
  EXPECT_EQ(swap_response->sources.size(), 1UL);
  EXPECT_EQ(swap_response->sources.at(0)->name, "Uniswap_V2");
  EXPECT_EQ(swap_response->sources.at(0)->proportion, "1");
  ASSERT_TRUE(swap_response->fees->zero_ex_fee);
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_type, "volume");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_token,
            "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_amount, "148470027512868522");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->billing_type, "on-chain");

  // Case 2: null zeroExFee
  json = R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources":[],
      "fees": {
        "zeroExFee": null
      }
    }
  )";
  swap_response = ParseSwapResponse(ParseJson(json), false);
  ASSERT_TRUE(swap_response);
  EXPECT_FALSE(swap_response->fees->zero_ex_fee);

  // Case 3: malformed fees field
  json = R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources":[],
      "fees": null
    }
  )";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), false));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), false));
  json = R"({"price": 3})";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), false));
  json = "3";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), false));
  json = "[3]";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), false));
  EXPECT_FALSE(ParseSwapResponse(base::Value(), false));
}

TEST(SwapResponseParserUnitTest, ParseTransactionPayload) {
  // Case 1: non-null zeroExFee
  std::string json(R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee": {
          "feeType": "volume",
          "feeToken": "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063",
          "feeAmount": "148470027512868522",
          "billingType": "on-chain"
        }
      }
    }
  )");
  mojom::SwapResponsePtr swap_response =
      ParseSwapResponse(ParseJson(json), true);
  ASSERT_TRUE(swap_response);

  EXPECT_EQ(swap_response->price, "1916.27547998814058355");
  EXPECT_EQ(swap_response->guaranteed_price, "1935.438234788021989386");
  EXPECT_EQ(swap_response->to, "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(swap_response->data, "0x0");

  EXPECT_EQ(swap_response->value, "0");
  EXPECT_EQ(swap_response->gas, "719000");
  EXPECT_EQ(swap_response->estimated_gas, "719001");
  EXPECT_EQ(swap_response->gas_price, "26000000000");
  EXPECT_EQ(swap_response->protocol_fee, "0");
  EXPECT_EQ(swap_response->minimum_protocol_fee, "0");
  EXPECT_EQ(swap_response->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(swap_response->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(swap_response->buy_amount, "1000000000000000000000");
  EXPECT_EQ(swap_response->sell_amount, "1916275479988140583549706");
  EXPECT_EQ(swap_response->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(swap_response->sell_token_to_eth_rate, "1900.44962824532464391");
  EXPECT_EQ(swap_response->buy_token_to_eth_rate, "1");
  EXPECT_EQ(swap_response->estimated_price_impact, "0.7232");
  EXPECT_EQ(swap_response->sources.size(), 1UL);
  EXPECT_EQ(swap_response->sources.at(0)->name, "Uniswap_V2");
  EXPECT_EQ(swap_response->sources.at(0)->proportion, "1");
  ASSERT_TRUE(swap_response->fees->zero_ex_fee);
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_type, "volume");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_token,
            "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->fee_amount, "148470027512868522");
  EXPECT_EQ(swap_response->fees->zero_ex_fee->billing_type, "on-chain");

  // Case 2: null zeroExFee
  json = R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources":[],
      "fees": {
        "zeroExFee": null
      }
    }
  )";
  swap_response = ParseSwapResponse(ParseJson(json), true);
  ASSERT_TRUE(swap_response);
  EXPECT_FALSE(swap_response->fees->zero_ex_fee);

  // Case 3: malformed fees field
  json = R"(
    {
      "price":"1916.27547998814058355",
      "guaranteedPrice":"1935.438234788021989386",
      "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "data":"0x0",
      "value":"0",
      "gas":"719000",
      "estimatedGas":"719001",
      "gasPrice":"26000000000",
      "protocolFee":"0",
      "minimumProtocolFee":"0",
      "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
      "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
      "buyAmount":"1000000000000000000000",
      "sellAmount":"1916275479988140583549706",
      "sources":[],
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources":[],
      "fees": null
    }
  )";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), true));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), true));
  json = R"({"price": 3})";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), true));
  json = "3";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), true));
  json = "[3]";
  EXPECT_FALSE(ParseSwapResponse(ParseJson(json), true));
  EXPECT_FALSE(ParseSwapResponse(base::Value(), true));
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuote) {
  auto* json_template = GetJupiterQuoteTemplate();
  std::string json = base::StringPrintf(json_template, "10000", "30");

  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);
  ASSERT_EQ(swap_quote->routes.size(), 1UL);
  ASSERT_EQ(swap_quote->routes.at(0)->in_amount, 10000ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->out_amount, 261273ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->amount, 10000ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->other_amount_threshold, 258660ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->swap_mode, "ExactIn");
  ASSERT_EQ(swap_quote->routes.at(0)->price_impact_pct, 0.008955716118219659);
  ASSERT_EQ(swap_quote->routes.at(0)->slippage_bps, 50);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.size(), 1UL);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->id,
            "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->label, "Orca");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->input_mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->output_mint,
            "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->not_enough_liquidity,
            false);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->in_amount, 10000ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->out_amount,
            117001203ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->price_impact_pct,
            1.196568750220778e-7);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->lp_fee->amount,
            30ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->lp_fee->mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->lp_fee->pct, 0.003);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->platform_fee->amount,
            0ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->platform_fee->mint,
            "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey");
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->platform_fee->pct, 0);

  // OK: Max uint64 amount value
  json = base::StringPrintf(json_template, "18446744073709551615", "30");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  // OK: Max uint64 for lpFee value
  json = base::StringPrintf(json_template, "10000", "18446744073709551615");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  // KO: Malformed quote
  ASSERT_FALSE(ParseJupiterQuote(base::Value()));

  // KO: Invalid quote
  ASSERT_FALSE(ParseJupiterQuote(ParseJson(R"({"price": "3"})")));

  // KO: uint64 amount value underflow
  json = base::StringPrintf(json_template, "-1", "30");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);

  // KO: uint64 amount value overflow (UINT64_MAX + 1)
  json = base::StringPrintf(json_template, "18446744073709551616", "30");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);

  // KO: Integer lpFee value underflow
  json = base::StringPrintf(json_template, "10000", "-1");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);

  // KO: Integer lpFee value overflow (UINT64_MAX + 1)
  json = base::StringPrintf(json_template, "10000", "18446744073709551616");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuoteSlippageBps) {
  auto* json_fmt(R"(
    {
      "data": [
        {
          "inAmount": "10000",
          "outAmount": "261273",
          "amount": "10000",
          "otherAmountThreshold": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": "1.1",
          "slippageBps": %s,
          "marketInfos": []
        }
      ],
      "timeTaken": "0.044471802000089156"
    })");

  // OK: valid slippageBps value
  std::string json = base::StringPrintf(json_fmt, "\"50\"");
  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);
  EXPECT_EQ(swap_quote->routes.at(0)->slippage_bps, 50);

  // KO: null slippageBps value
  json = base::StringPrintf(json_fmt, "null");
  EXPECT_FALSE(ParseJupiterQuote(ParseJson(json)));

  // KO: non-integer slippageBps value
  json = base::StringPrintf(json_fmt, "\"50.55\"");
  EXPECT_FALSE(ParseJupiterQuote(ParseJson(json)));
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuotePriceImpact) {
  auto* json_template = GetJupiterQuoteTemplateForPriceImpact();
  std::string json = base::StringPrintf(json_template, "\"1.1\"", "\"1.1\"");
  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);
  ASSERT_EQ(swap_quote->routes.at(0)->price_impact_pct, 1.1);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->price_impact_pct,
            1.1);

  json = base::StringPrintf(json_template, "null", "null");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);
  ASSERT_EQ(swap_quote->routes.at(0)->price_impact_pct, 0.0);
  ASSERT_EQ(swap_quote->routes.at(0)->market_infos.at(0)->price_impact_pct,
            0.0);

  json = base::StringPrintf(json_template, "123", "null");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);

  json = base::StringPrintf(json_template, "null", "123");
  swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_FALSE(swap_quote);
}

TEST(SwapResponseParserUnitTest, ParseJupiterSwapTransactions) {
  std::string json(R"(
    {
      "swapTransaction": "swap"
    })");

  auto transactions = ParseJupiterSwapTransactions(ParseJson(json));
  ASSERT_TRUE(transactions);
  ASSERT_EQ(transactions->swap_transaction, "swap");

  ASSERT_FALSE(ParseJupiterSwapTransactions(base::Value()));
  ASSERT_FALSE(ParseJupiterSwapTransactions(ParseJson(R"({"foo": "bar"})")));
}

TEST(SwapResponseParserUnitTest, ParseSwapErrorResponse) {
  {
    std::string json(R"(
    {
      "code": 100,
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "field": "buyAmount",
          "code": 1004,
          "reason": "INSUFFICIENT_ASSET_LIQUIDITY"
        }
      ]
    })");

    auto swap_error = ParseSwapErrorResponse(ParseJson(json));
    EXPECT_EQ(swap_error->code, 100);
    EXPECT_EQ(swap_error->reason, "Validation Failed");
    EXPECT_EQ(swap_error->validation_errors.size(), 1u);
    EXPECT_EQ(swap_error->validation_errors.front()->field, "buyAmount");
    EXPECT_EQ(swap_error->validation_errors.front()->code, 1004);
    EXPECT_EQ(swap_error->validation_errors.front()->reason,
              "INSUFFICIENT_ASSET_LIQUIDITY");

    EXPECT_TRUE(swap_error->is_insufficient_liquidity);
  }
  {
    std::string json(R"(
    {
      "code": 100,
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "field": "buyAmount",
          "code": 1004,
          "reason": "SOMETHING_ELSE"
        }
      ]
    })");

    auto swap_error = ParseSwapErrorResponse(ParseJson(json));
    EXPECT_EQ(swap_error->code, 100);
    EXPECT_EQ(swap_error->reason, "Validation Failed");
    EXPECT_EQ(swap_error->validation_errors.size(), 1u);
    EXPECT_EQ(swap_error->validation_errors.front()->field, "buyAmount");
    EXPECT_EQ(swap_error->validation_errors.front()->code, 1004);
    EXPECT_EQ(swap_error->validation_errors.front()->reason, "SOMETHING_ELSE");

    EXPECT_FALSE(swap_error->is_insufficient_liquidity);
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

    auto jupiter_error = ParseJupiterErrorResponse(ParseJson(json));
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

    auto jupiter_error = ParseJupiterErrorResponse(ParseJson(json));
    EXPECT_EQ(jupiter_error->status_code, "some code");
    EXPECT_EQ(jupiter_error->error, "error");
    EXPECT_EQ(jupiter_error->message, "some message");

    EXPECT_FALSE(jupiter_error->is_insufficient_liquidity);
  }
}

}  // namespace brave_wallet
