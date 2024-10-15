/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_response_parser.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet {

std::optional<std::string> ParseFilGetBalance(const base::Value& json_value) {
  return ParseSingleStringResult(json_value);
}

std::optional<uint64_t> ParseFilGetTransactionCount(
    const base::Value& json_value) {
  if (json_value.is_none()) {
    return std::nullopt;
  }

  auto count_string = ParseSingleStringResult(json_value);
  if (!count_string) {
    return std::nullopt;
  }
  if (count_string->empty()) {
    return std::nullopt;
  }

  uint64_t count = 0;
  if (!base::StringToUint64(*count_string, &count)) {
    return std::nullopt;
  }
  return count;
}

bool ParseFilEstimateGas(const base::Value& json_value,
                         std::string* gas_premium,
                         std::string* gas_fee_cap,
                         int64_t* gas_limit) {
  auto result = ParseResultDict(json_value);
  if (!result) {
    return false;
  }
  auto* limit = result->FindString("GasLimit");
  if (!limit) {
    return false;
  }
  auto* premium = result->FindString("GasPremium");
  if (!premium) {
    return false;
  }
  auto* fee_cap = result->FindString("GasFeeCap");
  if (!fee_cap) {
    return false;
  }
  if (!base::StringToInt64(*limit, gas_limit)) {
    return false;
  }
  *gas_fee_cap = *fee_cap;
  *gas_premium = *premium;
  return true;
}

bool ParseFilGetChainHead(const base::Value& json_value, uint64_t* height) {
  auto result = ParseResultDict(json_value);
  if (!height || !result) {
    return false;
  }
  auto* height_value = result->FindString("Height");
  if (!height_value) {
    return false;
  }

  return base::StringToUint64(*height_value, height);
}

// Returns parsed receipt exit code.
bool ParseFilStateSearchMsgLimited(const base::Value& json_value,
                                   const std::string& cid,
                                   int64_t* exit_code) {
  auto result = ParseResultDict(json_value);
  if (!exit_code || !result) {
    return false;
  }
  auto* cid_value = result->FindStringByDottedPath("Message./");
  if (!cid_value || cid != *cid_value) {
    return false;
  }
  auto* code_value = result->FindStringByDottedPath("Receipt.ExitCode");
  if (!code_value) {
    return false;
  }
  return base::StringToInt64(*code_value, exit_code);
}

bool ParseSendFilecoinTransaction(const base::Value& json_value,
                                  std::string* cid) {
  auto result = ParseResultDict(json_value);
  if (!cid || !result) {
    return false;
  }

  auto* cid_value = result->FindString("/");
  if (!cid_value) {
    return false;
  }
  *cid = *cid_value;
  return true;
}

}  // namespace brave_wallet
