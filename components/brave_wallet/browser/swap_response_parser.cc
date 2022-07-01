/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_response_parser.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace {

absl::optional<std::string> ParseResultFromDict(
    const base::Value::Dict& response_dict,
    const std::string& key) {
  const auto* val = response_dict.FindString(key);
  if (!val) {
    return absl::nullopt;
  }

  return *val;
}

absl::optional<uint64_t> ParseUint64ResultFromStringDictValue(
    const base::Value::Dict& dict_value,
    const std::string& key) {
  const auto* value = dict_value.FindString(key);
  if (!value)
    return absl::nullopt;

  uint64_t ret;
  if (base::StringToUint64(*value, &ret))
    return ret;

  return absl::nullopt;
}

absl::optional<double> ParseDoubleResultFromStringDictValue(
    const base::Value::Dict& dict_value,
    const std::string& key) {
  const auto* value = dict_value.FindString(key);
  if (!value)
    return absl::nullopt;

  double ret;
  if (base::StringToDouble(*value, &ret))
    return ret;

  return absl::nullopt;
}

absl::optional<const base::Value::List> GetRoutesFromJupiterSwapQuote(
    const std::string& json) {
  auto records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return absl::nullopt;
  }

  const auto& response_dict = records_v->GetDict();
  const auto* routes_value = response_dict.FindList("data");
  if (!routes_value)
    return absl::nullopt;

  return routes_value->Clone();
}

}  // namespace

namespace brave_wallet {

mojom::SwapResponsePtr ParseSwapResponse(const std::string& json,
                                         bool expect_transaction_data) {
  auto swap_response = mojom::SwapResponse::New();

  // {
  //   "price":"1916.27547998814058355",
  //   "guaranteedPrice":"1935.438234788021989386",
  //   "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
  //   "data":"...",
  //   "value":"0",
  //   "gas":"719000",
  //   "estimatedGas":"719000",
  //   "gasPrice":"26000000000",
  //   "protocolFee":"0",
  //   "minimumProtocolFee":"0",
  //   "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
  //   "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
  //   "buyAmount":"1000000000000000000000",
  //   "sellAmount":"1916275479988140583549706",
  //   "sources":[...],
  //   "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
  //   "sellTokenToEthRate":"1900.44962824532464391",
  //   "buyTokenToEthRate":"1"
  // }

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return nullptr;
  }

  const auto& response_dict = records_v->GetDict();

  auto price = ParseResultFromDict(response_dict, "price");
  if (!price)
    return nullptr;
  swap_response->price = *price;

  if (expect_transaction_data) {
    auto guaranteed_price =
        ParseResultFromDict(response_dict, "guaranteedPrice");
    if (!guaranteed_price)
      return nullptr;
    swap_response->guaranteed_price = *guaranteed_price;

    auto to = ParseResultFromDict(response_dict, "to");
    if (!to)
      return nullptr;
    swap_response->to = *to;

    auto data = ParseResultFromDict(response_dict, "data");
    if (!data)
      return nullptr;
    swap_response->data = *data;
  }

  auto value = ParseResultFromDict(response_dict, "value");
  if (!value)
    return nullptr;
  swap_response->value = *value;

  auto gas = ParseResultFromDict(response_dict, "gas");
  if (!gas)
    return nullptr;
  swap_response->gas = *gas;

  auto estimated_gas = ParseResultFromDict(response_dict, "estimatedGas");
  if (!estimated_gas)
    return nullptr;
  swap_response->estimated_gas = *estimated_gas;

  auto gas_price = ParseResultFromDict(response_dict, "gasPrice");
  if (!gas_price)
    return nullptr;
  swap_response->gas_price = *gas_price;

  auto protocol_fee = ParseResultFromDict(response_dict, "protocolFee");
  if (!protocol_fee)
    return nullptr;

  swap_response->protocol_fee = *protocol_fee;

  auto minimum_protocol_fee =
      ParseResultFromDict(response_dict, "minimumProtocolFee");
  if (!minimum_protocol_fee)
    return nullptr;

  swap_response->minimum_protocol_fee = *minimum_protocol_fee;

  auto buy_token_address =
      ParseResultFromDict(response_dict, "buyTokenAddress");
  if (!buy_token_address)
    return nullptr;
  swap_response->buy_token_address = *buy_token_address;

  auto sell_token_address =
      ParseResultFromDict(response_dict, "sellTokenAddress");
  if (!sell_token_address)
    return nullptr;
  swap_response->sell_token_address = *sell_token_address;

  auto buy_amount = ParseResultFromDict(response_dict, "buyAmount");
  if (!buy_amount)
    return nullptr;
  swap_response->buy_amount = *buy_amount;

  auto sell_amount = ParseResultFromDict(response_dict, "sellAmount");
  if (!sell_amount)
    return nullptr;
  swap_response->sell_amount = *sell_amount;

  auto allowance_target = ParseResultFromDict(response_dict, "allowanceTarget");
  if (!allowance_target)
    return nullptr;
  swap_response->allowance_target = *allowance_target;

  auto sell_token_to_eth_rate =
      ParseResultFromDict(response_dict, "sellTokenToEthRate");
  if (!sell_token_to_eth_rate)
    return nullptr;
  swap_response->sell_token_to_eth_rate = *sell_token_to_eth_rate;

  auto buy_token_to_eth_rate =
      ParseResultFromDict(response_dict, "buyTokenToEthRate");
  if (!buy_token_to_eth_rate)
    return nullptr;
  swap_response->buy_token_to_eth_rate = *buy_token_to_eth_rate;

  return swap_response;
}

mojom::JupiterQuotePtr ParseJupiterQuote(const std::string& json) {
  //    {
  //      "data": [
  //        {
  //          "inAmount": "10000",
  //          "outAmount": "261273",
  //          "amount": "10000",
  //          "otherAmountThreshold": "258660",
  //          "outAmountWithSlippage": "258660",
  //          "swapMode": "ExactIn",
  //          "priceImpactPct": "0.008955716118219659",
  //          "marketInfos": [
  //            {
  //              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
  //              "label": "Orca",
  //              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
  //              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
  //              "notEnoughLiquidity": false,
  //              "inAmount": "10000",
  //              "outAmount": "117001203",
  //              "priceImpactPct": "0.0000001196568750220778",
  //              "lpFee": {
  //                "amount": "30",
  //                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
  //                "pct": "0.003"
  //              },
  //              "platformFee": {
  //                "amount": "0",
  //                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
  //                "pct": "0"
  //              }
  //            }
  //          ]
  //        }
  //      ],
  //      "timeTaken": "0.044471802000089156"
  //    }
  const auto routes_value = GetRoutesFromJupiterSwapQuote(json);
  if (!routes_value)
    return nullptr;

  auto swap_quote = mojom::JupiterQuote::New();
  std::vector<mojom::JupiterRoutePtr> routes;

  for (const auto& route_value : *routes_value) {
    const auto& route_dict = route_value.GetDict();
    mojom::JupiterRoute route;

    auto in_amount =
        ParseUint64ResultFromStringDictValue(route_dict, "inAmount");
    if (!in_amount)
      return nullptr;
    route.in_amount = *in_amount;

    auto out_amount =
        ParseUint64ResultFromStringDictValue(route_dict, "outAmount");
    if (!out_amount)
      return nullptr;
    route.out_amount = *out_amount;

    auto amount = ParseUint64ResultFromStringDictValue(route_dict, "amount");
    if (!amount)
      return nullptr;
    route.amount = *amount;

    auto other_amount_threshold = ParseUint64ResultFromStringDictValue(
        route_dict, "otherAmountThreshold");
    if (!other_amount_threshold)
      return nullptr;
    route.other_amount_threshold = *other_amount_threshold;

    auto* swap_mode = route_dict.FindString("swapMode");
    if (!swap_mode)
      return nullptr;
    route.swap_mode = *swap_mode;

    auto price_impact_pct =
        ParseDoubleResultFromStringDictValue(route_dict, "priceImpactPct");
    if (!price_impact_pct)
      return nullptr;
    route.price_impact_pct = *price_impact_pct;

    const auto* market_infos_value = route_dict.FindList("marketInfos");
    if (!market_infos_value)
      return nullptr;

    for (const auto& market_info_value : *market_infos_value) {
      const auto& market_info_dict = market_info_value.GetDict();
      mojom::JupiterMarketInfo market_info;

      auto* market_info_id = market_info_dict.FindString("id");
      if (!market_info_id)
        return nullptr;
      market_info.id = *market_info_id;

      auto* market_info_label = market_info_dict.FindString("label");
      if (!market_info_label)
        return nullptr;
      market_info.label = *market_info_label;

      auto* market_info_input_mint = market_info_dict.FindString("inputMint");
      if (!market_info_input_mint)
        return nullptr;
      market_info.input_mint = *market_info_input_mint;

      auto* market_info_output_mint = market_info_dict.FindString("outputMint");
      if (!market_info_output_mint)
        return nullptr;
      market_info.output_mint = *market_info_output_mint;

      auto not_enough_liquidity =
          market_info_dict.FindBool("notEnoughLiquidity");
      if (!not_enough_liquidity)
        return nullptr;
      market_info.not_enough_liquidity = *not_enough_liquidity;

      auto market_info_in_amount =
          ParseUint64ResultFromStringDictValue(market_info_dict, "inAmount");
      if (!market_info_in_amount)
        return nullptr;
      market_info.in_amount = *market_info_in_amount;

      auto market_info_out_amount =
          ParseUint64ResultFromStringDictValue(market_info_dict, "outAmount");
      if (!market_info_out_amount)
        return nullptr;
      market_info.out_amount = *market_info_out_amount;

      auto market_info_price_impact_pct = ParseDoubleResultFromStringDictValue(
          market_info_dict, "priceImpactPct");
      if (!market_info_price_impact_pct)
        return nullptr;
      market_info.price_impact_pct = *market_info_price_impact_pct;

      const base::Value::Dict* lp_fee_value =
          market_info_dict.FindDict("lpFee");
      if (!lp_fee_value)
        return nullptr;

      // Parse lpFee->amount field as a JSON integer field, since the
      // values are typically very small, and intermediate conversion to string
      // is expensive due to its deep nesting.
      mojom::JupiterFee lp_fee;
      auto lp_fee_amount =
          ParseUint64ResultFromStringDictValue(*lp_fee_value, "amount");
      if (!lp_fee_amount)
        return nullptr;
      lp_fee.amount = *lp_fee_amount;

      auto* lp_fee_mint = lp_fee_value->FindString("mint");
      if (!lp_fee_mint)
        return nullptr;
      lp_fee.mint = *lp_fee_mint;

      auto lp_fee_pct =
          ParseDoubleResultFromStringDictValue(*lp_fee_value, "pct");
      if (!lp_fee_pct)
        return nullptr;
      lp_fee.pct = *lp_fee_pct;

      market_info.lp_fee = lp_fee.Clone();

      const base::Value::Dict* platform_fee_value =
          market_info_dict.FindDict("platformFee");
      if (!platform_fee_value)
        return nullptr;

      // Parse platformFee->amount field as a JSON integer field, since the
      // values are typically very small, and intermediate conversion to string
      // is expensive due to its deep nesting.
      mojom::JupiterFee platform_fee;
      auto platform_fee_amount =
          ParseUint64ResultFromStringDictValue(*platform_fee_value, "amount");
      if (!platform_fee_amount)
        return nullptr;
      platform_fee.amount = *platform_fee_amount;

      auto* platform_fee_mint = platform_fee_value->FindString("mint");
      if (!platform_fee_mint)
        return nullptr;
      platform_fee.mint = *platform_fee_mint;

      auto platform_fee_pct =
          ParseDoubleResultFromStringDictValue(*platform_fee_value, "pct");
      if (!platform_fee_pct)
        return nullptr;
      platform_fee.pct = *platform_fee_pct;

      market_info.platform_fee = platform_fee.Clone();

      route.market_infos.push_back(market_info.Clone());
    }

    swap_quote->routes.push_back(route.Clone());
  }

  return swap_quote;
}

mojom::JupiterSwapTransactionsPtr ParseJupiterSwapTransactions(
    const std::string& json) {
  auto swap_transactions = mojom::JupiterSwapTransactions::New();

  auto records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return nullptr;
  }

  const auto& response_dict = records_v->GetDict();

  auto setup_transaction =
      ParseResultFromDict(response_dict, "setupTransaction");
  if (!setup_transaction)
    swap_transactions->setup_transaction = "";
  else
    swap_transactions->setup_transaction = *setup_transaction;

  auto swap_transaction = ParseResultFromDict(response_dict, "swapTransaction");
  if (!swap_transaction)
    return nullptr;
  swap_transactions->swap_transaction = *swap_transaction;

  auto cleanup_transaction =
      ParseResultFromDict(response_dict, "cleanupTransaction");
  if (!cleanup_transaction)
    swap_transactions->cleanup_transaction = "";
  else
    swap_transactions->cleanup_transaction = *cleanup_transaction;

  return swap_transactions;
}

// Function to convert all numbers in JSON string to strings.
//
// For sample JSON response, refer to ParseJupiterQuote.
absl::optional<std::string> ConvertAllNumbersToString(const std::string& json) {
  auto converted_json = std::string(json::convert_all_numbers_to_string(json));
  if (converted_json.empty())
    return absl::nullopt;

  return converted_json;
}

}  // namespace brave_wallet
