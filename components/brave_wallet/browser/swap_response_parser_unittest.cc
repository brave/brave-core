/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

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
          "outAmountWithSlippage": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": "0.008955716118219659",
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
}  // namespace

TEST(SwapResponseParserUnitTest, ParsePriceQuote) {
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
      "buyTokenToEthRate":"1"
    }
  )");
  mojom::SwapResponsePtr swap_response = ParseSwapResponse(json, false);
  ASSERT_TRUE(swap_response);

  ASSERT_EQ(swap_response->price, "1916.27547998814058355");
  // ASSERT_EQ(swap_response->guaranteed_price, "1935.438234788021989386");
  // ASSERT_EQ(swap_response->to, "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  // ASSERT_EQ(swap_response->data, "0x0");
  ASSERT_TRUE(swap_response->guaranteed_price.empty());
  ASSERT_TRUE(swap_response->to.empty());
  ASSERT_TRUE(swap_response->data.empty());

  ASSERT_EQ(swap_response->value, "0");
  ASSERT_EQ(swap_response->gas, "719000");
  ASSERT_EQ(swap_response->estimated_gas, "719001");
  ASSERT_EQ(swap_response->gas_price, "26000000000");
  ASSERT_EQ(swap_response->protocol_fee, "0");
  ASSERT_EQ(swap_response->minimum_protocol_fee, "0");
  ASSERT_EQ(swap_response->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  ASSERT_EQ(swap_response->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  ASSERT_EQ(swap_response->buy_amount, "1000000000000000000000");
  ASSERT_EQ(swap_response->sell_amount, "1916275479988140583549706");
  ASSERT_EQ(swap_response->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  ASSERT_EQ(swap_response->sell_token_to_eth_rate, "1900.44962824532464391");
  ASSERT_EQ(swap_response->buy_token_to_eth_rate, "1");

  json = R"({"price": "3"})";
  ASSERT_FALSE(ParseSwapResponse(json, false));
  json = R"({"price": 3})";
  ASSERT_FALSE(ParseSwapResponse(json, false));
  json = "3";
  ASSERT_FALSE(ParseSwapResponse(json, false));
  json = "[3]";
  ASSERT_FALSE(ParseSwapResponse(json, false));
  json = "";
  ASSERT_FALSE(ParseSwapResponse(json, false));
}

TEST(SwapResponseParserUnitTest, ParseTransactionPayload) {
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
      "buyTokenToEthRate":"1"
    }
  )");
  mojom::SwapResponsePtr swap_response = ParseSwapResponse(json, true);
  ASSERT_TRUE(swap_response);

  ASSERT_EQ(swap_response->price, "1916.27547998814058355");
  ASSERT_EQ(swap_response->guaranteed_price, "1935.438234788021989386");
  ASSERT_EQ(swap_response->to, "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  ASSERT_EQ(swap_response->data, "0x0");

  ASSERT_EQ(swap_response->value, "0");
  ASSERT_EQ(swap_response->gas, "719000");
  ASSERT_EQ(swap_response->estimated_gas, "719001");
  ASSERT_EQ(swap_response->gas_price, "26000000000");
  ASSERT_EQ(swap_response->protocol_fee, "0");
  ASSERT_EQ(swap_response->minimum_protocol_fee, "0");
  ASSERT_EQ(swap_response->buy_token_address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  ASSERT_EQ(swap_response->sell_token_address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  ASSERT_EQ(swap_response->buy_amount, "1000000000000000000000");
  ASSERT_EQ(swap_response->sell_amount, "1916275479988140583549706");
  ASSERT_EQ(swap_response->allowance_target,
            "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
  ASSERT_EQ(swap_response->sell_token_to_eth_rate, "1900.44962824532464391");
  ASSERT_EQ(swap_response->buy_token_to_eth_rate, "1");

  json = R"({"price": "3"})";
  ASSERT_FALSE(ParseSwapResponse(json, true));
  json = R"({"price": 3})";
  ASSERT_FALSE(ParseSwapResponse(json, true));
  json = "3";
  ASSERT_FALSE(ParseSwapResponse(json, true));
  json = "[3]";
  ASSERT_FALSE(ParseSwapResponse(json, true));
  json = "";
  ASSERT_FALSE(ParseSwapResponse(json, true));
}

TEST(SwapResponseParserUnitTest, ParseJupiterQuote) {
  auto* json_template = GetJupiterQuoteTemplate();
  std::string json = base::StringPrintf(json_template, "10000", "30");

  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(json);
  ASSERT_TRUE(swap_quote);
  ASSERT_EQ(swap_quote->routes.size(), 1UL);
  ASSERT_EQ(swap_quote->routes.at(0)->in_amount, 10000ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->out_amount, 261273ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->amount, 10000ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->other_amount_threshold, 258660ULL);
  ASSERT_EQ(swap_quote->routes.at(0)->swap_mode, "ExactIn");
  ASSERT_EQ(swap_quote->routes.at(0)->price_impact_pct, 0.008955716118219659);
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
  swap_quote = ParseJupiterQuote(json);
  ASSERT_TRUE(swap_quote);

  // OK: Max uint64 for lpFee value
  json = base::StringPrintf(json_template, "10000", "18446744073709551615");
  swap_quote = ParseJupiterQuote(json);
  ASSERT_TRUE(swap_quote);

  // KO: Malformed quote
  ASSERT_FALSE(ParseJupiterQuote(""));

  // KO: Invalid quote
  ASSERT_FALSE(ParseJupiterQuote(R"({"price": "3"})"));

  // KO: uint64 amount value underflow
  json = base::StringPrintf(json_template, "-1", "30");
  swap_quote = ParseJupiterQuote(json);
  ASSERT_FALSE(swap_quote);

  // KO: uint64 amount value overflow (UINT64_MAX + 1)
  json = base::StringPrintf(json_template, "18446744073709551616", "30");
  swap_quote = ParseJupiterQuote(json);
  ASSERT_FALSE(swap_quote);

  // KO: Integer lpFee value underflow
  json = base::StringPrintf(json_template, "10000", "-1");
  swap_quote = ParseJupiterQuote(json);
  ASSERT_FALSE(swap_quote);

  // KO: Integer lpFee value overflow (UINT64_MAX + 1)
  json = base::StringPrintf(json_template, "10000", "18446744073709551616");
  swap_quote = ParseJupiterQuote(json);
  ASSERT_FALSE(swap_quote);
}

TEST(SwapResponseParserUnitTest, ParseJupiterSwapTransactions) {
  std::string json(R"(
    {
      "setupTransaction": "setup",
      "swapTransaction": "swap",
      "cleanupTransaction": "cleanup"
    })");

  auto swap_transactions = ParseJupiterSwapTransactions(json);
  ASSERT_TRUE(swap_transactions);
  ASSERT_EQ(swap_transactions->setup_transaction, "setup");
  ASSERT_EQ(swap_transactions->swap_transaction, "swap");
  ASSERT_EQ(swap_transactions->cleanup_transaction, "cleanup");

  ASSERT_FALSE(ParseJupiterSwapTransactions(""));
  ASSERT_FALSE(ParseJupiterSwapTransactions(R"({"foo": "bar"})"));
}

}  // namespace brave_wallet
