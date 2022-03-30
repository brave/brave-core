/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/bls/rs/src/lib.rs.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

namespace {

bool IsNumericString(const std::string& value) {
  return std::all_of(value.begin(), value.end(),
                     [](char c) { return std::isdigit(c) != 0; });
}

}  // namespace

FilTransaction::FilTransaction() = default;

FilTransaction::FilTransaction(const FilTransaction&) = default;
FilTransaction::FilTransaction(absl::optional<uint64_t> nonce,
                               const std::string& gas_premium,
                               const std::string& gas_fee_cap,
                               int64_t gas_limit,
                               const std::string& max_fee,
                               const FilAddress& to,
                               const FilAddress& from,
                               const std::string& value,
                               const std::string& cid)
    : nonce_(nonce),
      gas_premium_(gas_premium),
      gas_fee_cap_(gas_fee_cap),
      gas_limit_(gas_limit),
      max_fee_(max_fee),
      cid_(cid),
      to_(to),
      from_(from),
      value_(value) {}

FilTransaction::~FilTransaction() = default;

bool FilTransaction::IsEqual(const FilTransaction& tx) const {
  return nonce_ == tx.nonce_ && gas_premium_ == tx.gas_premium_ &&
         gas_fee_cap_ == tx.gas_fee_cap_ && gas_limit_ == tx.gas_limit_ &&
         max_fee_ == tx.max_fee_ && to_ == tx.to_ && from_ == tx.from_ &&
         value_ == tx.value_ && cid_ == tx.cid_;
}

bool FilTransaction::operator==(const FilTransaction& other) const {
  return IsEqual(other);
}

bool FilTransaction::operator!=(const FilTransaction& other) const {
  return !IsEqual(other);
}

// static
absl::optional<FilTransaction> FilTransaction::FromTxData(
    const mojom::FilTxDataPtr& tx_data) {
  FilTransaction tx;
  uint64_t nonce = 0;
  if (!tx_data->nonce.empty() && base::StringToUint64(tx_data->nonce, &nonce)) {
    tx.nonce_ = nonce;
  }

  auto address = FilAddress::FromAddress(tx_data->to);
  if (address.IsEmpty())
    return absl::nullopt;
  tx.to_ = address;

  auto from = FilAddress::FromAddress(tx_data->from);
  if (from.IsEmpty())
    return absl::nullopt;
  tx.from_ = from;

  if (tx_data->value.empty() || !IsNumericString(tx_data->value))
    return absl::nullopt;
  tx.set_value(tx_data->value);

  if (!IsNumericString(tx_data->gas_fee_cap))
    return absl::nullopt;
  tx.set_fee_cap(tx_data->gas_fee_cap);

  if (!IsNumericString(tx_data->gas_premium))
    return absl::nullopt;
  tx.set_gas_premium(tx_data->gas_premium);

  if (!IsNumericString(tx_data->max_fee))
    return absl::nullopt;
  tx.set_max_fee(tx_data->max_fee);

  tx.set_cid(tx_data->cid);

  int64_t gas_limit = 0;
  if (!tx_data->gas_limit.empty()) {
    if (!base::StringToInt64(tx_data->gas_limit, &gas_limit))
      return absl::nullopt;
  }
  tx.set_gas_limit(gas_limit);

  return tx;
}

base::Value FilTransaction::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("nonce",
                    nonce_ ? base::NumberToString(nonce_.value()) : "");
  dict.SetStringKey("gaspremium", gas_premium_);
  dict.SetStringKey("gasfeecap", gas_fee_cap_);
  dict.SetStringKey("maxfee", max_fee_);
  dict.SetStringKey("gaslimit", base::NumberToString(gas_limit_));
  dict.SetStringKey("to", to_.EncodeAsString());
  dict.SetStringKey("from", from_.EncodeAsString());
  dict.SetStringKey("value", value_);

  base::Value cid(base::Value::Type::DICTIONARY);
  cid.SetStringKey("/", cid_);
  dict.SetKey("CID", std::move(cid));
  return dict;
}

// static
absl::optional<FilTransaction> FilTransaction::FromValue(
    const base::Value& value) {
  FilTransaction tx;
  const std::string* nonce_value = value.FindStringKey("nonce");
  if (!nonce_value)
    return absl::nullopt;

  if (!nonce_value->empty()) {
    uint64_t nonce = 0;
    if (!base::StringToUint64(*nonce_value, &nonce))
      return absl::nullopt;
    tx.nonce_ = nonce;
  }

  const std::string* gas_premium = value.FindStringKey("gaspremium");
  if (!gas_premium)
    return absl::nullopt;
  tx.gas_premium_ = *gas_premium;

  const std::string* gas_fee_cap = value.FindStringKey("gasfeecap");
  if (!gas_fee_cap)
    return absl::nullopt;
  tx.gas_fee_cap_ = *gas_fee_cap;

  const std::string* max_fee = value.FindStringKey("maxfee");
  if (!max_fee)
    return absl::nullopt;
  tx.max_fee_ = *max_fee;

  const std::string* gas_limit = value.FindStringKey("gaslimit");
  if (!gas_limit || !base::StringToInt64(*gas_limit, &tx.gas_limit_))
    return absl::nullopt;

  const std::string* to = value.FindStringKey("to");
  if (!to)
    return absl::nullopt;
  tx.to_ = FilAddress::FromAddress(*to);

  const std::string* tx_value = value.FindStringKey("value");
  if (!tx_value)
    return absl::nullopt;
  tx.value_ = *tx_value;

  const base::Value* cid_root = value.FindDictKey("CID");
  if (cid_root) {
    auto* cid_node = cid_root->FindStringKey("/");
    if (cid_node)
      tx.cid_ = *cid_node;
  }

  return tx;
}

std::string FilTransaction::GetMessageToSign() const {
  auto value = ToValue();
  value.RemoveKey("CID");
  value.RemoveKey("maxfee");
  value.SetIntKey("method", 0);
  value.SetStringKey("params", "");

  std::string json;
  base::JSONWriter::Write(value, &json);
  std::string converted_json =
      json::convert_string_value_to_int64("/gaslimit", json.c_str()).c_str();
  if (converted_json.empty())
    return std::string();
  converted_json =
      json::convert_string_value_to_uint64("/nonce", converted_json.c_str())
          .c_str();
  return converted_json;
}

std::string FilTransaction::GetSignedTransaction(
    const std::string& private_key_base64) const {
  return std::string(
      bls::fil_transaction_sign(GetMessageToSign(), private_key_base64)
          .c_str());
}

}  // namespace brave_wallet
