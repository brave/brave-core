/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_response_parser.h"

#include <limits>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"

namespace brave_wallet {

namespace solana {

bool ParseGetBalance(const std::string& json, uint64_t* balance) {
  DCHECK(balance);

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  // uint64
  const base::Value* value = result.FindKey("value");
  if (!value)
    return false;

  if (value->is_int() || value->is_double()) {
    std::string balance_string = base::NumberToString(value->GetDouble());
    return base::StringToUint64(balance_string, balance);
  }

  return false;
}

bool ParseGetTokenAccountBalance(const std::string& json,
                                 std::string* amount,
                                 uint8_t* decimals,
                                 std::string* ui_amount_string) {
  DCHECK(amount && decimals && ui_amount_string);

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  base::Value* value = result.FindDictKey("value");
  if (!value)
    return false;

  auto* amount_ptr = value->FindStringKey("amount");
  if (!amount_ptr)
    return false;
  *amount = *amount_ptr;

  // uint8
  auto decimals_opt = value->FindIntKey("decimals");
  if (!decimals_opt ||
      decimals_opt.value() > std::numeric_limits<uint8_t>::max() ||
      decimals_opt.value() < 0)
    return false;
  *decimals = decimals_opt.value();

  auto* ui_amount_string_ptr = value->FindStringKey("uiAmountString");
  if (!ui_amount_string_ptr)
    return false;
  *ui_amount_string = *ui_amount_string_ptr;

  return true;
}

}  // namespace solana

}  // namespace brave_wallet
