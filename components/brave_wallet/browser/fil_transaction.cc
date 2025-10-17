/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/adapters.h"
#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/json/json_helper.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

namespace {
// https://github.com/filecoin-project/go-state-types/blob/95828685f9df463f052a5d42b8f6c2502f873ceb/crypto/signature.go#L17
// https://spec.filecoin.io/algorithms/crypto/signatures/#section-algorithms.crypto.signatures.signature-types
enum SigType { ECDSASigType = 1, BLSSigType = 2 };

bool IsNumericString(const std::string& value) {
  return std::all_of(value.begin(), value.end(),
                     [](char c) { return std::isdigit(c) != 0; });
}

std::optional<std::vector<uint8_t>> ToBigIntBytesArray(
    const std::string& value) {
  auto bigint = Base10ValueToUint256(value);
  if (!bigint) {
    return std::nullopt;
  }

  std::vector<uint8_t> result;

  // Zero bigint is encoded as an empty byte array, Positive bigint has zero
  // byte prefix and bytes in big endian order.
  // https://github.com/filecoin-project/ref-fvm/blob/280d80503f950a1934e3d60910d659fad685f9c7/shared/src/bigint/biguint_ser.rs#L29-L34
  if (*bigint == 0) {
    return result;
  }

  uint32_t significant_bytes_count = 32 - (__builtin_clzg(*bigint) / 8);
  result.reserve(significant_bytes_count + 1);
  result.push_back(0);

  auto bigint_span =
      base::byte_span_from_ref(*bigint).first(significant_bytes_count);
  for (auto item : base::Reversed(bigint_span)) {
    result.push_back(item);
  }

  return result;
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
    auto eth_address = EthAddress::FromHex(tx_data->to);
    if (!eth_address.IsValid()) {
      return std::nullopt;
    }
    address = FilAddress::FromFEVMAddress(is_mainnet, eth_address);
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

base::Value::Dict FilTransaction::GetMessageToSign(
    const FilAddress& from) const {
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
  return value;
}

std::optional<std::vector<uint8_t>> FilTransaction::GetMessageToSignCBOR(
    const FilAddress& from) const {
  auto value_bytes = ToBigIntBytesArray(value_);
  auto gas_fee_cap_bytes = ToBigIntBytesArray(gas_fee_cap_);
  auto gas_premium_bytes = ToBigIntBytesArray(gas_premium_);
  if (!value_bytes || !gas_fee_cap_bytes || !gas_premium_bytes) {
    return std::nullopt;
  }

  std::vector<cbor::Value> data_vec;
  data_vec.emplace_back(0);  // Version.
  data_vec.emplace_back(to_.GetBytesForCbor());
  data_vec.emplace_back(from.GetBytesForCbor());
  data_vec.emplace_back(static_cast<int64_t>(nonce_.value_or(0)));
  data_vec.emplace_back(*value_bytes);
  data_vec.emplace_back(gas_limit_);
  data_vec.emplace_back(*gas_fee_cap_bytes);
  data_vec.emplace_back(*gas_premium_bytes);
  if (to_.protocol() == mojom::FilecoinAddressProtocol::DELEGATED) {
    // https://github.com/filecoin-project/FIPs/blob/master/FIPS/fip-0054.md#invokecontract-method-number-38444508371
    data_vec.emplace_back(int64_t(3844450837));  // Method.
  } else {
    data_vec.emplace_back(0);  // Method.
  }
  data_vec.emplace_back(base::span<const uint8_t>());  // Params.

  auto encoded_sig_data = cbor::Writer::Write(cbor::Value(std::move(data_vec)));
  CHECK(encoded_sig_data.has_value());

  return encoded_sig_data.value();
}

std::optional<std::array<uint8_t, kFilTransactionCidSize>>
FilTransaction::TransactionCid(const FilAddress& from) const {
  auto cbor = GetMessageToSignCBOR(from);
  if (!cbor) {
    return std::nullopt;
  }

  std::array<uint8_t, kFilTransactionCidSize> cid = {0x01, 0x71, 0xa0,
                                                     0xe4, 0x02, 0x20};
  base::span(cid).last<32>().copy_from(Blake2bHash<32>({*cbor}));

  return cid;
}

std::optional<std::string> FilTransaction::GetMessageToSignJson(
    const FilAddress& from) const {
  std::string json;
  if (!base::JSONWriter::Write(GetMessageToSign(from), &json)) {
    return std::nullopt;
  }

  return ConvertMessageStringFieldsToInt64("", json);
}

// static
std::optional<std::string> FilTransaction::ConvertMessageStringFieldsToInt64(
    const std::string& path,
    const std::string& json) {
  std::string converted_json =
      json::convert_string_value_to_int64(path + "/GasLimit", json, true);
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
  return ConvertMessageStringFieldsToInt64(path + "/Message", json);
}

// static
std::optional<base::Value::Dict> FilTransaction::DeserializeSignedTx(
    const std::string& signed_tx) {
  std::string json =
      json::convert_int64_value_to_string("/Message/GasLimit", signed_tx, true);
  json = json::convert_int64_value_to_string("/Message/Nonce", json, true);
  json = json::convert_int64_value_to_string("/Message/Method", json, true);
  return base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
}

// https://spec.filecoin.io/algorithms/crypto/signatures/#section-algorithms.crypto.signatures
std::optional<std::string> FilTransaction::GetSignedTransaction(
    const FilAddress& from,
    base::span<const uint8_t> signature) const {
  DCHECK(!from.IsEmpty());
  DCHECK(from.protocol() == mojom::FilecoinAddressProtocol::SECP256K1 ||
         from.protocol() == mojom::FilecoinAddressProtocol::BLS)
      << from.protocol();

  auto message = GetMessageToSign(from);
  base::Value::Dict signature_dict;
  signature_dict.Set("Data", base::Base64Encode(signature));
  signature_dict.Set("Type",
                     from.protocol() == mojom::FilecoinAddressProtocol::BLS
                         ? BLSSigType
                         : ECDSASigType);
  base::Value::Dict dict;
  dict.Set("Message", std::move(message));
  dict.Set("Signature", std::move(signature_dict));
  std::string json;
  if (!base::JSONWriter::Write(dict, &json)) {
    return std::nullopt;
  }
  return ConvertMessageStringFieldsToInt64("/Message", json);
}

mojom::FilTxDataPtr FilTransaction::ToFilTxData() const {
  return mojom::FilTxData::New(nonce() ? base::NumberToString(*nonce()) : "",
                               gas_premium(), gas_fee_cap(),
                               base::NumberToString(gas_limit()), max_fee(),
                               to().EncodeAsString(), value());
}

}  // namespace brave_wallet
