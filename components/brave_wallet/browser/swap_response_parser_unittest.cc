/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SwapResponseParserUnitTest, ParsePriceQuote) {
  auto swap_response = mojom::SwapResponse::New();
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
  std::string price;
  ASSERT_TRUE(ParseSwapResponse(json, false, &swap_response));

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
  ASSERT_FALSE(ParseSwapResponse(json, false, &swap_response));
  json = R"({"price": 3})";
  ASSERT_FALSE(ParseSwapResponse(json, false, &swap_response));
  json = "3";
  ASSERT_FALSE(ParseSwapResponse(json, false, &swap_response));
  json = "[3]";
  ASSERT_FALSE(ParseSwapResponse(json, false, &swap_response));
  json = "";
  ASSERT_FALSE(ParseSwapResponse(json, false, &swap_response));
}

TEST(SwapResponseParserUnitTest, ParseTransactionPayload) {
  auto swap_response = mojom::SwapResponse::New();
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
  std::string price;
  ASSERT_TRUE(ParseSwapResponse(json, true, &swap_response));

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
  ASSERT_FALSE(ParseSwapResponse(json, true, &swap_response));
  json = R"({"price": 3})";
  ASSERT_FALSE(ParseSwapResponse(json, true, &swap_response));
  json = "3";
  ASSERT_FALSE(ParseSwapResponse(json, true, &swap_response));
  json = "[3]";
  ASSERT_FALSE(ParseSwapResponse(json, true, &swap_response));
  json = "";
  ASSERT_FALSE(ParseSwapResponse(json, true, &swap_response));
}

}  // namespace brave_wallet
