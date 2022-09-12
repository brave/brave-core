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

bool GetUint64FromDictValue(const base::Value::Dict& dict_value,
                            const std::string& key,
                            bool nullable,
                            uint64_t* ret) {
  if (!ret) {
    return false;
  }

  const base::Value* value = dict_value.Find(key);
  if (!value)
    return false;

  if (nullable && value->is_none()) {
    *ret = 0;
    return true;
  }

  auto* string_value = value->GetIfString();
  if (!string_value || string_value->empty())
    return false;

  return base::StringToUint64(*string_value, ret);
}

}  // namespace

namespace solana {

bool ParseGetBalance(const std::string& json, uint64_t* balance) {
  DCHECK(balance);

  auto result = ParseResultDict(json);
  if (!result || !result)
    return false;

  return GetUint64FromDictValue(*result, "value", false, balance);
}

bool ParseGetTokenAccountBalance(const std::string& json,
                                 std::string* amount,
                                 uint8_t* decimals,
                                 std::string* ui_amount_string) {
  DCHECK(amount && decimals && ui_amount_string);

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  auto* value = result->FindDict("value");
  if (!value)
    return false;

  auto* amount_ptr = value->FindString("amount");
  if (!amount_ptr)
    return false;
  *amount = *amount_ptr;

  // uint8
  auto decimals_opt = value->FindInt("decimals");
  if (!decimals_opt ||
      decimals_opt.value() > std::numeric_limits<uint8_t>::max() ||
      decimals_opt.value() < 0)
    return false;
  *decimals = decimals_opt.value();

  auto* ui_amount_string_ptr = value->FindString("uiAmountString");
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

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  auto* value = result->FindDict("value");
  if (!value)
    return false;

  auto* hash_ptr = value->FindString("blockhash");
  if (!hash_ptr || hash_ptr->empty())
    return false;
  *hash = *hash_ptr;

  return GetUint64FromDictValue(*value, "lastValidBlockHeight", false,
                                last_valid_block_height);
}

bool ParseGetSignatureStatuses(
    const std::string& json,
    std::vector<absl::optional<SolanaSignatureStatus>>* statuses) {
  DCHECK(statuses);
  statuses->clear();

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  const auto* value = result->FindList("value");
  if (!value)
    return false;

  for (const auto& item : *value) {
    const auto* status_value = item.GetIfDict();
    if (!status_value) {
      statuses->push_back(absl::nullopt);
      continue;
    }

    SolanaSignatureStatus status;
    if (!GetUint64FromDictValue(*status_value, "slot", false, &status.slot) ||
        !GetUint64FromDictValue(*status_value, "confirmations", true,
                                &status.confirmations)) {
      statuses->push_back(absl::nullopt);
      continue;
    }

    const base::Value* err_value = status_value->Find("err");
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
        status_value->Find("confirmationStatus");
    if (!confirmation_status_value ||
        (!confirmation_status_value->is_string() &&
         !confirmation_status_value->is_none())) {
      statuses->push_back(absl::nullopt);
    } else if (confirmation_status_value->is_none()) {
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

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  const base::Value* value = result->Find("value");
  if (!value || (!value->is_none() && !value->is_dict()))
    return false;

  if (value->is_none()) {
    *account_info_out = absl::nullopt;
    return true;
  }

  SolanaAccountInfo account_info;
  const auto& value_dict = value->GetDict();
  if (!GetUint64FromDictValue(value_dict, "lamports", false,
                              &account_info.lamports))
    return false;

  auto* owner = value_dict.FindString("owner");
  if (!owner)
    return false;
  account_info.owner = *owner;

  auto* data = value_dict.FindList("data");
  if (!data)
    return false;
  if (data->size() != 2u || !(*data)[0].is_string() ||
      !(*data)[1].is_string() || (*data)[1].GetString() != "base64")
    return false;
  auto data_string = (*data)[0].GetString();
  if (!base::Base64Decode(data_string))
    return false;
  account_info.data = data_string;

  auto executable = value_dict.FindBool("executable");
  if (!executable)
    return false;
  account_info.executable = *executable;

  if (!GetUint64FromDictValue(value_dict, "rentEpoch", false,
                              &account_info.rent_epoch))
    return false;
  *account_info_out = account_info;

  return true;
}

bool ParseGetFeeForMessage(const std::string& json, uint64_t* fee) {
  DCHECK(fee);

  auto result = ParseResultDict(json);
  if (!result)
    return false;

  return GetUint64FromDictValue(*result, "value", true, fee);
}

bool ParseGetBlockHeight(const std::string& json, uint64_t* block_height) {
  DCHECK(block_height);

  std::string block_height_string;
  if (!brave_wallet::ParseSingleStringResult(json, &block_height_string))
    return false;

  if (block_height_string.empty())
    return false;

  return base::StringToUint64(block_height_string, block_height);
}

}  // namespace solana

}  // namespace brave_wallet
