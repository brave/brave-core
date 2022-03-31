/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_response_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace {

bool ParseResultFromDict(const base::DictionaryValue* response_dict,
                         const std::string& key,
                         std::string* output_val) {
  auto* val = response_dict->FindStringKey(key);
  if (!val) {
    return false;
  }
  *output_val = *val;
  return true;
}

}  // namespace

namespace brave_wallet {

bool ParseSwapResponse(const std::string& json,
                       bool expect_transaction_data,
                       mojom::SwapResponsePtr* swap_response) {
  DCHECK(swap_response);
  *swap_response = mojom::SwapResponse::New();
  auto& response = *swap_response;

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

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  auto& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  if (!ParseResultFromDict(response_dict, "price", &response->price))
    return false;

  if (expect_transaction_data) {
    if (!ParseResultFromDict(response_dict, "guaranteedPrice",
                             &response->guaranteed_price))
      return false;

    if (!ParseResultFromDict(response_dict, "to", &response->to))
      return false;

    if (!ParseResultFromDict(response_dict, "data", &response->data))
      return false;
  }

  if (!ParseResultFromDict(response_dict, "value", &response->value))
    return false;

  if (!ParseResultFromDict(response_dict, "gas", &response->gas))
    return false;

  if (!ParseResultFromDict(response_dict, "estimatedGas",
                           &response->estimated_gas))
    return false;

  if (!ParseResultFromDict(response_dict, "gasPrice", &response->gas_price))
    return false;

  if (!ParseResultFromDict(response_dict, "protocolFee",
                           &response->protocol_fee))
    return false;

  if (!ParseResultFromDict(response_dict, "minimumProtocolFee",
                           &response->minimum_protocol_fee))
    return false;

  if (!ParseResultFromDict(response_dict, "buyTokenAddress",
                           &response->buy_token_address))
    return false;

  if (!ParseResultFromDict(response_dict, "sellTokenAddress",
                           &response->sell_token_address))
    return false;

  if (!ParseResultFromDict(response_dict, "buyAmount", &response->buy_amount))
    return false;

  if (!ParseResultFromDict(response_dict, "sellAmount", &response->sell_amount))
    return false;

  if (!ParseResultFromDict(response_dict, "allowanceTarget",
                           &response->allowance_target))
    return false;

  if (!ParseResultFromDict(response_dict, "sellTokenToEthRate",
                           &response->sell_token_to_eth_rate))
    return false;

  if (!ParseResultFromDict(response_dict, "buyTokenToEthRate",
                           &response->buy_token_to_eth_rate))
    return false;

  return true;
}

}  // namespace brave_wallet
