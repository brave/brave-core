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
      zeroex::ParseQuoteResponse(ParseJson(json), false);
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
  quote = zeroex::ParseQuoteResponse(ParseJson(json), false);
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
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), false));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), false));
  json = R"({"price": 3})";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), false));
  json = "3";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), false));
  json = "[3]";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), false));
  EXPECT_FALSE(zeroex::ParseQuoteResponse(base::Value(), false));
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
  mojom::ZeroExQuotePtr quote =
      zeroex::ParseQuoteResponse(ParseJson(json), true);
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
  quote = zeroex::ParseQuoteResponse(ParseJson(json), true);
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
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), true));

  // Case 4: other invalid cases
  json = R"({"price": "3"})";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), true));
  json = R"({"price": 3})";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), true));
  json = "3";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), true));
  json = "[3]";
  EXPECT_FALSE(zeroex::ParseQuoteResponse(ParseJson(json), true));
  EXPECT_FALSE(zeroex::ParseQuoteResponse(base::Value(), true));
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
      "code": "100",
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "field": "buyAmount",
          "code": "1004",
          "reason": "INSUFFICIENT_ASSET_LIQUIDITY"
        }
      ]
    })");

    auto swap_error = zeroex::ParseErrorResponse(ParseJson(json));
    EXPECT_EQ(swap_error->code, "100");
    EXPECT_EQ(swap_error->reason, "Validation Failed");
    EXPECT_EQ(swap_error->validation_errors.size(), 1u);
    EXPECT_EQ(swap_error->validation_errors.front()->field, "buyAmount");
    EXPECT_EQ(swap_error->validation_errors.front()->code, "1004");
    EXPECT_EQ(swap_error->validation_errors.front()->reason,
              "INSUFFICIENT_ASSET_LIQUIDITY");

    EXPECT_TRUE(swap_error->is_insufficient_liquidity);
  }
  {
    std::string json(R"(
    {
      "code": "100",
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "field": "buyAmount",
          "code": "1004",
          "reason": "SOMETHING_ELSE"
        }
      ]
    })");

    auto swap_error = zeroex::ParseErrorResponse(ParseJson(json));
    EXPECT_EQ(swap_error->code, "100");
    EXPECT_EQ(swap_error->reason, "Validation Failed");
    EXPECT_EQ(swap_error->validation_errors.size(), 1u);
    EXPECT_EQ(swap_error->validation_errors.front()->field, "buyAmount");
    EXPECT_EQ(swap_error->validation_errors.front()->code, "1004");
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
    EXPECT_EQ(step->type, mojom::LiFiStepType::kNative);
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

}  // namespace brave_wallet
