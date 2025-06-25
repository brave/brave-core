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

std::optional<ParseFilEstimateGasResult> ParseFilEstimateGas(
    const base::Value& json_value) {
  auto result = ParseResultDict(json_value);
  if (!result) {
    return std::nullopt;
  }
  auto* limit = result->FindString("GasLimit");
  if (!limit) {
    return std::nullopt;
  }
  auto* premium = result->FindString("GasPremium");
  if (!premium) {
    return std::nullopt;
  }
  auto* fee_cap = result->FindString("GasFeeCap");
  if (!fee_cap) {
    return std::nullopt;
  }
  int64_t gas_limit = 0;
  if (!base::StringToInt64(*limit, &gas_limit)) {
    return std::nullopt;
  }

  ParseFilEstimateGasResult parse_gas_result;
  parse_gas_result.gas_fee_cap = *fee_cap;
  parse_gas_result.gas_premium = *premium;
  parse_gas_result.gas_limit = gas_limit;
  return parse_gas_result;
}

std::optional<uint64_t> ParseFilGetChainHead(const base::Value& json_value) {
  auto result = ParseResultDict(json_value);
  if (!result) {
    return std::nullopt;
  }
  auto* height_value = result->FindString("Height");
  if (!height_value) {
    return std::nullopt;
  }

  uint64_t height = 0;
  if (!base::StringToUint64(*height_value, &height)) {
    return std::nullopt;
  }
  return height;
}

// Returns parsed receipt exit code.
std::optional<int64_t> ParseFilStateSearchMsgLimited(
    const base::Value& json_value,
    const std::string& cid) {
  auto result = ParseResultDict(json_value);
  if (!result) {
    return std::nullopt;
  }
  auto* cid_value = result->FindStringByDottedPath("Message./");
  if (!cid_value || cid != *cid_value) {
    return std::nullopt;
  }
  auto* code_value = result->FindStringByDottedPath("Receipt.ExitCode");
  if (!code_value) {
    return std::nullopt;
  }
  int64_t exit_code = 0;
  if (!base::StringToInt64(*code_value, &exit_code)) {
    return std::nullopt;
  }
  return exit_code;
}

std::optional<std::string> ParseSendFilecoinTransaction(
    const base::Value& json_value) {
  auto result = ParseResultDict(json_value);
  if (!result) {
    return std::nullopt;
  }
  auto* cid_value = result->FindString("/");
  if (!cid_value) {
    return std::nullopt;
  }
  return *cid_value;
}

}  // namespace brave_wallet
