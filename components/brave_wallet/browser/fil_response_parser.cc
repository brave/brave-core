/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_response_parser.h"
#include <stdint.h>

#include <charconv>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

bool ParseFilGetBalance(const std::string& json, std::string* balance) {
  return brave_wallet::ParseSingleStringResult(json, balance);
}

bool ParseFilGetTransactionCount(const std::string& json, uint64_t* count) {
  base::Value result;
  if (!ParseResult(json, &result))
    return false;
  if (result.is_int() || result.is_double()) {
    std::string value_string = base::NumberToString(result.GetDouble());
    return base::StringToUint64(value_string, count);
  }
  return false;
}

bool ParseFilEstimateGas(const std::string& json,
                         std::string* gas_premium,
                         std::string* gas_fee_cap,
                         uint64_t* gas_limit,
                         std::string* cid) {
  base::Value result;
  if (!ParseResult(json, &result))
    return false;
  const base::DictionaryValue* result_dict = nullptr;
  if (!result.GetAsDictionary(&result_dict))
    return false;
  auto limit = result_dict->FindDoubleKey("GasLimit");
  if (!limit)
    return false;
  auto* premium = result_dict->FindStringKey("GasPremium");
  if (!premium)
    return false;
  auto* fee_cap = result_dict->FindStringKey("GasFeeCap");
  if (!fee_cap)
    return false;
  auto* cid_value = result_dict->FindKey("CID");
  if (!cid_value)
    return false;
  auto* cid_root = cid_value->FindStringKey("/");
  if (!cid_root)
    return false;

  *gas_fee_cap = *fee_cap;
  *gas_limit = *limit;
  *gas_premium = *premium;
  *cid = *cid_root;
  return true;
}

}  // namespace brave_wallet
