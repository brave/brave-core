/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_response_parser.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

bool ParseFilGetBalance(const std::string& json, std::string* balance) {
  return brave_wallet::ParseSingleStringResult(json, balance);
}

bool ParseFilGetTransactionCount(const std::string& raw_json, uint64_t* count) {
  DCHECK(count);

  if (raw_json.empty())
    return false;

  std::string count_string;
  if (!brave_wallet::ParseSingleStringResult(raw_json, &count_string))
    return false;
  if (count_string.empty())
    return false;
  return base::StringToUint64(count_string, count);
}

bool ParseFilEstimateGas(const std::string& json,
                         std::string* gas_premium,
                         std::string* gas_fee_cap,
                         int64_t* gas_limit) {
  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;
  auto* limit = result.FindStringKey("GasLimit");
  if (!limit)
    return false;
  auto* premium = result.FindStringKey("GasPremium");
  if (!premium)
    return false;
  auto* fee_cap = result.FindStringKey("GasFeeCap");
  if (!fee_cap)
    return false;
  if (!base::StringToInt64(*limit, gas_limit))
    return false;
  *gas_fee_cap = *fee_cap;
  *gas_premium = *premium;
  return true;
}

}  // namespace brave_wallet
