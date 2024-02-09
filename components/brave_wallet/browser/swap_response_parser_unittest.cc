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
  mojom::ZeroExQuotePtr quote =
      ParseZeroExQuoteResponse(ParseJson(json), false);
  ASSERT_TRUE(quote);

  EXPECT_EQ(quote->price, "1916.27547998814058355");
  EXPECT_TRUE(quote->guaranteed_price.empty());
  EXPECT_TRUE(quote->to.empty());
  EXPECT_TRUE(quote->data.empty());

  EXPECT_EQ(quote->value, "0");
  EXPECT_EQ(quote->gas, "719000");
  EXPECT_EQ(quote->estimated_gas, "719001");
  EXPECT_EQ(quote->gas_price, "26000000000");
  EXPECT_EQ(quote->protocol_fee, "0");
  EXPECT_EQ(quote->minimum_protocol_fee, "0");
  EXPECT_EQ(quote->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(quote->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(quote->buy_amount, "1000000000000000000000");
  EXPECT_EQ(quote->sell_amount, "1916275479988140583549706");
  EXPECT_EQ(quote->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(quote->sell_token_to_eth_rate, "1900.44962824532464391");
  EXPECT_EQ(quote->buy_token_to_eth_rate, "1");
  EXPECT_EQ(quote->estimated_price_impact, "0.7232");
  EXPECT_EQ(quote->sources.size(), 1UL);
  EXPECT_EQ(quote->sources.at(0)->name, "Uniswap_V2");
  EXPECT_EQ(quote->sources.at(0)->proportion, "1");
  ASSERT_TRUE(quote->fees->zero_ex_fee);
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_type, "volume");
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_token,
            "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063");
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_amount, "148470027512868522");
  EXPECT_EQ(quote->fees->zero_ex_fee->billing_type, "on-chain");

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
  quote = ParseZeroExQuoteResponse(ParseJson(json), false);
  ASSERT_TRUE(quote);
  EXPECT_FALSE(quote->fees->zero_ex_fee);

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
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), false));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), false));
  json = R"({"price": 3})";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), false));
  json = "3";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), false));
  json = "[3]";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), false));
  EXPECT_FALSE(ParseZeroExQuoteResponse(base::Value(), false));
}

TEST(SwapResponseParserUnitTest, ParseZeroExTransactionResponse) {
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
  mojom::ZeroExQuotePtr quote = ParseZeroExQuoteResponse(ParseJson(json), true);
  ASSERT_TRUE(quote);

  EXPECT_EQ(quote->price, "1916.27547998814058355");
  EXPECT_EQ(quote->guaranteed_price, "1935.438234788021989386");
  EXPECT_EQ(quote->to, "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(quote->data, "0x0");

  EXPECT_EQ(quote->value, "0");
  EXPECT_EQ(quote->gas, "719000");
  EXPECT_EQ(quote->estimated_gas, "719001");
  EXPECT_EQ(quote->gas_price, "26000000000");
  EXPECT_EQ(quote->protocol_fee, "0");
  EXPECT_EQ(quote->minimum_protocol_fee, "0");
  EXPECT_EQ(quote->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(quote->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(quote->buy_amount, "1000000000000000000000");
  EXPECT_EQ(quote->sell_amount, "1916275479988140583549706");
  EXPECT_EQ(quote->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  EXPECT_EQ(quote->sell_token_to_eth_rate, "1900.44962824532464391");
  EXPECT_EQ(quote->buy_token_to_eth_rate, "1");
  EXPECT_EQ(quote->estimated_price_impact, "0.7232");
  EXPECT_EQ(quote->sources.size(), 1UL);
  EXPECT_EQ(quote->sources.at(0)->name, "Uniswap_V2");
  EXPECT_EQ(quote->sources.at(0)->proportion, "1");
  ASSERT_TRUE(quote->fees->zero_ex_fee);
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_type, "volume");
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_token,
            "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063");
  EXPECT_EQ(quote->fees->zero_ex_fee->fee_amount, "148470027512868522");
  EXPECT_EQ(quote->fees->zero_ex_fee->billing_type, "on-chain");

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
  quote = ParseZeroExQuoteResponse(ParseJson(json), true);
  ASSERT_TRUE(quote);
  EXPECT_FALSE(quote->fees->zero_ex_fee);

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
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), true));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), true));
  json = R"({"price": 3})";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), true));
  json = "3";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), true));
  json = "[3]";
  EXPECT_FALSE(ParseZeroExQuoteResponse(ParseJson(json), true));
  EXPECT_FALSE(ParseZeroExQuoteResponse(base::Value(), true));
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuoteResponse) {
  auto* json = GetJupiterQuoteResponse();
  mojom::JupiterQuotePtr swap_quote =
      ParseJupiterQuoteResponse(ParseJson(json));

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
  EXPECT_TRUE(ParseJupiterQuoteResponse(ParseJson(R"({
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
  EXPECT_FALSE(ParseJupiterQuoteResponse(base::Value()));

  // KO: Invalid quote
  EXPECT_FALSE(ParseJupiterQuoteResponse(ParseJson(R"({"price": "3"})")));

  // KO: Invalid platformFee value
  EXPECT_FALSE(ParseJupiterQuoteResponse(ParseJson(R"({
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

  auto transaction = ParseJupiterTransactionResponse(ParseJson(json));
  ASSERT_TRUE(transaction);
  ASSERT_EQ(transaction, "swap");

  ASSERT_FALSE(ParseJupiterTransactionResponse(base::Value()));
  ASSERT_FALSE(ParseJupiterTransactionResponse(ParseJson(R"({"foo": "bar"})")));
}

TEST(SwapResponseParserUnitTest, ParseZeroExErrorResponse) {
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

    auto swap_error = ParseZeroExErrorResponse(ParseJson(json));
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

    auto swap_error = ParseZeroExErrorResponse(ParseJson(json));
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
