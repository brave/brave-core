/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_response_parser.h"

#include <limits>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

namespace {

bool GetUint64FromDictValue(const base::Value& dict_value,
                            const std::string& key,
                            bool nullable,
                            uint64_t* ret) {
  if (!dict_value.is_dict() || !ret) {
    return false;
  }

  const base::Value* value = dict_value.FindKey(key);
  if (!value)
    return false;

  if (nullable && value->is_none()) {
    *ret = 0;
    return true;
  }

  if (!value->is_int() && !value->is_double()) {
    return false;
  }

  // We currently only support number up to kMaxSafeIntegerUint64, because
  // double-precision floating-point can only precisely represent an integer
  // up to kMaxSafeIntegerUint64.
  double double_value = value->GetDouble();
  if (double_value < 0 || double_value > kMaxSafeIntegerUint64)
    return false;
  *ret = static_cast<uint64_t>(double_value);

  // This will be false if double_value is not an integer, which is considered
  // as an invalid input.
  return double_value == static_cast<double>(*ret);
}

}  // namespace

namespace solana {

bool ParseGetBalance(const std::string& json, uint64_t* balance) {
  DCHECK(balance);

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  return GetUint64FromDictValue(result, "value", false, balance);
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

bool ParseSendTransaction(const std::string& json, std::string* tx_id) {
  return ParseSingleStringResult(json, tx_id);
}

bool ParseGetLatestBlockhash(const std::string& json,
                             std::string* hash,
                             uint64_t* last_valid_block_height) {
  DCHECK(hash && last_valid_block_height);

  std::string converted_json(json::convert_uint64_value_to_string(
      "/result/value/lastValidBlockHeight", json));
  if (converted_json.empty())
    return false;

  base::Value result;
  if (!ParseResult(converted_json, &result) || !result.is_dict())
    return false;

  base::Value* value = result.FindDictKey("value");
  if (!value)
    return false;

  auto* hash_ptr = value->FindStringKey("blockhash");
  if (!hash_ptr || hash_ptr->empty())
    return false;
  *hash = *hash_ptr;

  auto* last_valid_block_height_string =
      value->FindStringKey("lastValidBlockHeight");
  if (!last_valid_block_height_string ||
      last_valid_block_height_string->empty())
    return false;
  return base::StringToUint64(*last_valid_block_height_string,
                              last_valid_block_height);
}

bool ParseGetSignatureStatuses(
    const std::string& json,
    std::vector<absl::optional<SolanaSignatureStatus>>* statuses) {
  DCHECK(statuses);
  statuses->clear();

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  const base::Value* value = result.FindListKey("value");
  if (!value)
    return false;

  for (const auto& status_value : value->GetList()) {
    if (!status_value.is_dict()) {
      statuses->push_back(absl::nullopt);
      continue;
    }

    SolanaSignatureStatus status;
    if (!GetUint64FromDictValue(status_value, "slot", false, &status.slot) ||
        !GetUint64FromDictValue(status_value, "confirmations", true,
                                &status.confirmations)) {
      statuses->push_back(absl::nullopt);
      continue;
    }

    const base::Value* err_value = status_value.FindKey("err");
    if (!err_value || (!err_value->is_dict() && !err_value->is_none())) {
      statuses->push_back(absl::nullopt);
      continue;
    }
    if (err_value->is_none()) {
      status.err = "";
    } else {
      base::JSONWriter::Write(*err_value, &status.err);
    }

    const base::Value* confirmation_status_value =
        status_value.FindKey("confirmationStatus");
    if (!confirmation_status_value ||
        (!confirmation_status_value->is_string() &&
         !confirmation_status_value->is_none())) {
      statuses->push_back(absl::nullopt);
    }

    if (confirmation_status_value->is_none()) {
      status.confirmation_status = "";
    } else {  // is_string
      const std::string* confirmation_status =
          confirmation_status_value->GetIfString();
      DCHECK(confirmation_status);
      status.confirmation_status = *confirmation_status;
    }

    statuses->push_back(status);
  }

  return true;
}

bool ParseGetAccountInfo(const std::string& json,
                         absl::optional<SolanaAccountInfo>* account_info_out) {
  DCHECK(account_info_out);

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  const base::Value* value = result.FindKey("value");
  if (!value || (!value->is_none() && !value->is_dict()))
    return false;

  if (value->is_none()) {
    *account_info_out = absl::nullopt;
    return true;
  }

  SolanaAccountInfo account_info;
  if (!GetUint64FromDictValue(*value, "lamports", false,
                              &account_info.lamports))
    return false;

  auto* owner = value->FindStringKey("owner");
  if (!owner)
    return false;
  account_info.owner = *owner;

  auto* data = value->FindListKey("data");
  if (!data)
    return false;
  const auto& data_list = data->GetList();
  if (data_list.size() != 2u || !data_list[0].is_string() ||
      !data_list[1].is_string() || data_list[1].GetString() != "base64")
    return false;
  auto data_string = data_list[0].GetString();
  if (!base::Base64Decode(data_string))
    return false;
  account_info.data = data_string;

  auto executable = value->FindBoolKey("executable");
  if (!executable)
    return false;
  account_info.executable = *executable;

  if (!GetUint64FromDictValue(*value, "rentEpoch", false,
                              &account_info.rent_epoch))
    return false;
  *account_info_out = account_info;

  return true;
}

bool ParseGetFeeForMessage(const std::string& json, uint64_t* fee) {
  DCHECK(fee);

  base::Value result;
  if (!ParseResult(json, &result) || !result.is_dict())
    return false;

  return GetUint64FromDictValue(result, "value", true, fee);
}

bool ParseGetBlockHeight(const std::string& json, uint64_t* block_height) {
  DCHECK(block_height);

  absl::optional<std::string> converted_json = ConvertSingleUint64Result(json);
  std::string block_height_string;
  if (!converted_json || !brave_wallet::ParseSingleStringResult(
                             *converted_json, &block_height_string))
    return false;

  if (block_height_string.empty())
    return false;

  return base::StringToUint64(block_height_string, block_height);
}

}  // namespace solana

}  // namespace brave_wallet
