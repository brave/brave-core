/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet {

namespace {
// https://github.com/filecoin-project/go-state-types/blob/95828685f9df463f052a5d42b8f6c2502f873ceb/crypto/signature.go#L17
// https://spec.filecoin.io/algorithms/crypto/signatures/#section-algorithms.crypto.signatures.signature-types
enum SigType { ECDSASigType = 1, BLSSigType = 2 };

bool IsNumericString(const std::string& value) {
  return std::all_of(value.begin(), value.end(),
                     [](char c) { return std::isdigit(c) != 0; });
}

}  // namespace

FilTransaction::FilTransaction() = default;

FilTransaction::FilTransaction(const FilTransaction&) = default;
FilTransaction::FilTransaction(std::optional<uint64_t> nonce,
                               const std::string& gas_premium,
                               const std::string& gas_fee_cap,
                               int64_t gas_limit,
                               const std::string& max_fee,
                               const FilAddress& to,
                               const std::string& value)
    : nonce_(nonce),
      gas_premium_(gas_premium),
      gas_fee_cap_(gas_fee_cap),
      gas_limit_(gas_limit),
      max_fee_(max_fee),
      to_(to),
      value_(value) {}

FilTransaction::~FilTransaction() = default;

bool FilTransaction::IsEqual(const FilTransaction& tx) const {
  return nonce_ == tx.nonce_ && gas_premium_ == tx.gas_premium_ &&
         gas_fee_cap_ == tx.gas_fee_cap_ && gas_limit_ == tx.gas_limit_ &&
         max_fee_ == tx.max_fee_ && to_ == tx.to_ && value_ == tx.value_;
}

bool FilTransaction::operator==(const FilTransaction& other) const {
  return IsEqual(other);
}

bool FilTransaction::operator!=(const FilTransaction& other) const {
  return !IsEqual(other);
}

// static
std::optional<FilTransaction> FilTransaction::FromTxData(
    bool is_mainnet,
    const mojom::FilTxDataPtr& tx_data) {
  FilTransaction tx;
  uint64_t nonce = 0;
  if (!tx_data->nonce.empty() && base::StringToUint64(tx_data->nonce, &nonce)) {
    tx.nonce_ = nonce;
  }

  auto address = FilAddress::FromAddress(tx_data->to);
  if (address.IsEmpty()) {
    address = FilAddress::FromFEVMAddress(is_mainnet, tx_data->to);
    if (address.IsEmpty()) {
      return std::nullopt;
    }
  }
  tx.to_ = address;

  if (tx_data->value.empty() || !IsNumericString(tx_data->value)) {
    return std::nullopt;
  }
  tx.set_value(tx_data->value);

  if (!IsNumericString(tx_data->gas_fee_cap)) {
    return std::nullopt;
  }
  tx.set_fee_cap(tx_data->gas_fee_cap);

  if (!IsNumericString(tx_data->gas_premium)) {
    return std::nullopt;
  }
  tx.set_gas_premium(tx_data->gas_premium);

  if (!IsNumericString(tx_data->max_fee)) {
    return std::nullopt;
  }
  tx.set_max_fee(tx_data->max_fee);

  int64_t gas_limit = 0;
  if (!tx_data->gas_limit.empty()) {
    if (!base::StringToInt64(tx_data->gas_limit, &gas_limit)) {
      return std::nullopt;
    }
  }
  tx.set_gas_limit(gas_limit);

  return tx;
}

base::Value::Dict FilTransaction::ToValue() const {
  base::Value::Dict dict;
  dict.Set("Nonce", nonce_ ? base::NumberToString(nonce_.value()) : "");
  dict.Set("GasPremium", gas_premium_);
  dict.Set("GasFeeCap", gas_fee_cap_);
  dict.Set("MaxFee", max_fee_);
  dict.Set("GasLimit", base::NumberToString(gas_limit_));
  dict.Set("To", to_.EncodeAsString());
  dict.Set("Value", value_);
  return dict;
}

// static
std::optional<FilTransaction> FilTransaction::FromValue(
    const base::Value::Dict& value) {
  FilTransaction tx;
  const std::string* nonce_value = value.FindString("Nonce");
  if (!nonce_value) {
    return std::nullopt;
  }

  if (!nonce_value->empty()) {
    uint64_t nonce = 0;
    if (!base::StringToUint64(*nonce_value, &nonce)) {
      return std::nullopt;
    }
    tx.nonce_ = nonce;
  }

  const std::string* gas_premium = value.FindString("GasPremium");
  if (!gas_premium) {
    return std::nullopt;
  }
  tx.gas_premium_ = *gas_premium;

  const std::string* gas_fee_cap = value.FindString("GasFeeCap");
  if (!gas_fee_cap) {
    return std::nullopt;
  }
  tx.gas_fee_cap_ = *gas_fee_cap;

  const std::string* max_fee = value.FindString("MaxFee");
  if (!max_fee) {
    return std::nullopt;
  }
  tx.max_fee_ = *max_fee;

  const std::string* gas_limit = value.FindString("GasLimit");
  if (!gas_limit || !base::StringToInt64(*gas_limit, &tx.gas_limit_)) {
    return std::nullopt;
  }

  const std::string* to = value.FindString("To");
  if (!to) {
    return std::nullopt;
  }
  tx.to_ = FilAddress::FromAddress(*to);

  const std::string* tx_value = value.FindString("Value");
  if (!tx_value) {
    return std::nullopt;
  }
  tx.value_ = *tx_value;
  return tx;
}

base::Value FilTransaction::GetMessageToSign(const FilAddress& from) const {
  DCHECK(!from.IsEmpty());

  auto value = ToValue();
  value.Remove("MaxFee");
  if (to_.protocol() == mojom::FilecoinAddressProtocol::DELEGATED) {
    // https://github.com/filecoin-project/FIPs/blob/master/FIPS/fip-0054.md#invokecontract-method-number-38444508371
    value.Set("Method", "3844450837");
  } else {
    value.Set("Method", "0");
  }
  value.Set("From", from.EncodeAsString());
  value.Set("Version", 0);
  value.Set("Params", "");
  const std::string* nonce_value = value.FindString("Nonce");
  bool nonce_empty = nonce_value && nonce_value->empty();
  // Nonce is empty usually for first transactions. We set it to 0
  if (nonce_empty) {
    value.Set("Nonce", "0");
  }
  return base::Value(std::move(value));
}

std::optional<std::string> FilTransaction::GetMessageToSignJson(
    const FilAddress& from) const {
  std::string json;
  if (!base::JSONWriter::Write(GetMessageToSign(from), &json)) {
    return std::nullopt;
  }

  return ConvertMesssageStringFieldsToInt64("", json);
}

// static
std::optional<std::string> FilTransaction::ConvertMesssageStringFieldsToInt64(
    const std::string& path,
    const std::string& json) {
  std::string converted_json =
      json::convert_string_value_to_int64(path + "/GasLimit", json, true)
          .c_str();
  converted_json = json::convert_string_value_to_uint64(path + "/Nonce",
                                                        converted_json, true);
  converted_json = json::convert_string_value_to_uint64(path + "/Method",
                                                        converted_json, true);
  if (converted_json.empty()) {
    return std::nullopt;
  }
  return converted_json;
}

// static
std::optional<std::string> FilTransaction::ConvertSignedTxStringFieldsToInt64(
    const std::string& path,
    const std::string& json) {
  return ConvertMesssageStringFieldsToInt64(path + "/Message", json);
}

// static
std::optional<base::Value> FilTransaction::DeserializeSignedTx(
    const std::string& signed_tx) {
  std::string json =
      json::convert_int64_value_to_string("/Message/GasLimit", signed_tx, true);
  json = json::convert_int64_value_to_string("/Message/Nonce", json, true);
  json = json::convert_int64_value_to_string("/Message/Method", json, true);
  return base::JSONReader::Read(json);
}

// https://spec.filecoin.io/algorithms/crypto/signatures/#section-algorithms.crypto.signatures
std::optional<std::string> FilTransaction::GetSignedTransaction(
    const FilAddress& from,
    base::span<const uint8_t> private_key) const {
  DCHECK(!from.IsEmpty());

  auto message = GetMessageToSign(from);
  auto message_json = GetMessageToSignJson(from);
  if (!message_json) {
    return std::nullopt;
  }
  base::Value::Dict signature;
  {
    std::string data(filecoin::transaction_sign(
        from.network() == mojom::kFilecoinMainnet, *message_json,
        rust::Slice<const uint8_t>{private_key.data(), private_key.size()}));
    if (data.empty()) {
      return std::nullopt;
    }
    signature.Set("Data", data);
  }
  // Set signature type based on protocol.
  // https://spec.filecoin.io/algorithms/crypto/signatures/#section-algorithms.crypto.signatures.signature-types
  auto sig_type = from.protocol() == mojom::FilecoinAddressProtocol::SECP256K1
                      ? SigType::ECDSASigType
                      : SigType::BLSSigType;
  signature.Set("Type", sig_type);
  base::Value::Dict dict;
  dict.Set("Message", std::move(message));
  dict.Set("Signature", std::move(signature));
  std::string json;
  if (!base::JSONWriter::Write(dict, &json)) {
    return std::nullopt;
  }
  return ConvertMesssageStringFieldsToInt64("/Message", json);
}

mojom::FilTxDataPtr FilTransaction::ToFilTxData() const {
  return mojom::FilTxData::New(nonce() ? base::NumberToString(*nonce()) : "",
                               gas_premium(), gas_fee_cap(),
                               base::NumberToString(gas_limit()), max_fee(),
                               to().EncodeAsString(), value());
}

}  // namespace brave_wallet
