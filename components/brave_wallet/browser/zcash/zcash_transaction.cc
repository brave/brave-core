/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

#include <string_view>
#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {

bool ReadStringTo(const base::Value::Dict& dict,
                  std::string_view key,
                  std::string& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  to = *str;
  return true;
}

bool ReadUint64StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint64_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint64(*str, &to);
}

bool ReadUint32StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint32_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint(*str, &to);
}

template <class T>
bool ReadDictTo(const base::Value::Dict& dict, std::string_view key, T& to) {
  auto* key_dict = dict.FindDict(key);
  if (!key_dict) {
    return false;
  }
  auto t_opt = T::FromValue(*key_dict);
  if (!t_opt) {
    return false;
  }
  to = std::move(*t_opt);
  return true;
}

bool ReadHexByteArrayTo(const base::Value::Dict& dict,
                        std::string_view key,
                        std::vector<uint8_t>& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  if (str->empty()) {
    to.clear();
    return true;
  }
  return base::HexStringToBytes(*str, &to);
}

}  // namespace

ZCashTransaction::ZCashTransaction() = default;
ZCashTransaction::~ZCashTransaction() = default;
ZCashTransaction::ZCashTransaction(ZCashTransaction&& other) = default;
ZCashTransaction& ZCashTransaction::operator=(ZCashTransaction&& other) =
    default;
bool ZCashTransaction::operator==(const ZCashTransaction& other) const {
  return std::tie(this->inputs_, this->outputs_, this->locktime_, this->to_,
                  this->amount_, this->fee_) ==
         std::tie(other.inputs_, other.outputs_, other.locktime_, other.to_,
                  other.amount_, other.fee_);
}
bool ZCashTransaction::operator!=(const ZCashTransaction& other) const {
  return !(*this == other);
}

ZCashTransaction::Outpoint::Outpoint() = default;
ZCashTransaction::Outpoint::~Outpoint() = default;
ZCashTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
ZCashTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    Outpoint&& other) = default;
bool ZCashTransaction::Outpoint::operator==(
    const ZCashTransaction::Outpoint& other) const {
  return std::tie(this->txid, this->index) == std::tie(other.txid, other.index);
}
bool ZCashTransaction::Outpoint::operator!=(
    const ZCashTransaction::Outpoint& other) const {
  return !(*this == other);
}

base::Value::Dict ZCashTransaction::Outpoint::ToValue() const {
  base::Value::Dict dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", static_cast<int>(index));

  return dict;
}

// static
absl::optional<ZCashTransaction::Outpoint>
ZCashTransaction::Outpoint::FromValue(const base::Value::Dict& value) {
  Outpoint result;

  auto* txid_hex = value.FindString("txid");
  if (!txid_hex) {
    return absl::nullopt;
  }
  if (!base::HexStringToSpan(*txid_hex, result.txid)) {
    return absl::nullopt;
  }

  auto index_value = value.FindInt("index");
  if (!index_value) {
    return absl::nullopt;
  }
  result.index = *index_value;

  return result;
}

ZCashTransaction::TxInput::TxInput() = default;
ZCashTransaction::TxInput::~TxInput() = default;
ZCashTransaction::TxInput::TxInput(ZCashTransaction::TxInput&& other) = default;
ZCashTransaction::TxInput& ZCashTransaction::TxInput::operator=(
    ZCashTransaction::TxInput&& other) = default;
bool ZCashTransaction::TxInput::operator==(
    const ZCashTransaction::TxInput& other) const {
  return std::tie(this->utxo_address, this->utxo_outpoint, this->utxo_value,
                  this->script_sig, this->witness) ==
         std::tie(other.utxo_address, other.utxo_outpoint, other.utxo_value,
                  other.script_sig, other.witness);
}
bool ZCashTransaction::TxInput::operator!=(
    const ZCashTransaction::TxInput& other) const {
  return !(*this == other);
}

ZCashTransaction::TxInput ZCashTransaction::TxInput::Clone() const {
  ZCashTransaction::TxInput result;

  result.utxo_address = utxo_address;
  result.utxo_outpoint = utxo_outpoint;
  result.utxo_value = utxo_value;
  result.script_sig = script_sig;
  result.witness = witness;

  return result;
}

base::Value::Dict ZCashTransaction::TxInput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("utxo_address", utxo_address);
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));

  dict.Set("script_sig", base::HexEncode(script_sig));
  dict.Set("witness", base::HexEncode(witness));

  return dict;
}

// static
absl::optional<ZCashTransaction::TxInput> ZCashTransaction::TxInput::FromValue(
    const base::Value::Dict& value) {
  ZCashTransaction::TxInput result;

  if (!ReadStringTo(value, "utxo_address", result.utxo_address)) {
    return absl::nullopt;
  }

  if (!ReadDictTo(value, "utxo_outpoint", result.utxo_outpoint)) {
    return absl::nullopt;
  }

  if (!ReadUint64StringTo(value, "utxo_value", result.utxo_value)) {
    return absl::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_sig", result.script_sig)) {
    return absl::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "witness", result.witness)) {
    return absl::nullopt;
  }

  return result;
}

// static
absl::optional<ZCashTransaction::TxInput>
ZCashTransaction::TxInput::FromRpcUtxo(const std::string& address,
                                       const zcash::ZCashUtxo& utxo) {
  ZCashTransaction::TxInput result;
  result.utxo_address = address;
  // result.utxo_outpoint.txid = utxo.txid();
  result.utxo_outpoint.index = utxo.index();
  result.utxo_value = utxo.valuezat();
  return result;
}

uint32_t ZCashTransaction::TxInput::n_sequence() const {
  // Fixed value by now.
  // https://github.com/ZCash/ZCash/blob/v24.0/src/wallet/spend.cpp#L945
  return 0xfffffffd;
}

bool ZCashTransaction::TxInput::IsSigned() const {
  return !script_sig.empty() || !witness.empty();
}

ZCashTransaction::TxOutput::TxOutput() = default;
ZCashTransaction::TxOutput::~TxOutput() = default;
ZCashTransaction::TxOutput::TxOutput(ZCashTransaction::TxOutput&& other) =
    default;
ZCashTransaction::TxOutput& ZCashTransaction::TxOutput::operator=(
    ZCashTransaction::TxOutput&& other) = default;
bool ZCashTransaction::TxOutput::operator==(
    const ZCashTransaction::TxOutput& other) const {
  return std::tie(this->address, this->amount) ==
         std::tie(other.address, other.amount);
}
bool ZCashTransaction::TxOutput::operator!=(
    const ZCashTransaction::TxOutput& other) const {
  return !(*this == other);
}

ZCashTransaction::TxOutput ZCashTransaction::TxOutput::Clone() const {
  ZCashTransaction::TxOutput result;

  result.address = address;
  result.amount = amount;

  return result;
}

base::Value::Dict ZCashTransaction::TxOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("address", address);
  dict.Set("amount", base::NumberToString(amount));

  return dict;
}

// static
absl::optional<ZCashTransaction::TxOutput>
ZCashTransaction::TxOutput::FromValue(const base::Value::Dict& value) {
  ZCashTransaction::TxOutput result;
  if (!ReadStringTo(value, "address", result.address)) {
    return absl::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return absl::nullopt;
  }

  return result;
}

ZCashTransaction ZCashTransaction::Clone() const {
  ZCashTransaction result;

  for (auto& input : inputs_) {
    result.inputs_.emplace_back(input.Clone());
  }
  for (auto& output : outputs_) {
    result.outputs_.emplace_back(output.Clone());
  }
  result.locktime_ = locktime_;
  result.to_ = to_;
  result.amount_ = amount_;
  result.fee_ = fee_;

  return result;
}

base::Value::Dict ZCashTransaction::ToValue() const {
  base::Value::Dict dict;

  auto& inputs_value = dict.Set("inputs", base::Value::List())->GetList();
  for (auto& input : inputs_) {
    inputs_value.Append(input.ToValue());
  }

  auto& outputs_value = dict.Set("outputs", base::Value::List())->GetList();
  for (auto& output : outputs_) {
    outputs_value.Append(output.ToValue());
  }

  dict.Set("locktime", base::NumberToString(locktime_));
  dict.Set("to", to_);
  dict.Set("amount", base::NumberToString(amount_));
  dict.Set("fee", base::NumberToString(fee_));

  return dict;
}

// static
absl::optional<ZCashTransaction> ZCashTransaction::FromValue(
    const base::Value::Dict& value) {
  ZCashTransaction result;

  auto* inputs_list = value.FindList("inputs");
  if (!inputs_list) {
    return absl::nullopt;
  }
  for (auto& item : *inputs_list) {
    if (!item.is_dict()) {
      return absl::nullopt;
    }
    auto input_opt = ZCashTransaction::TxInput::FromValue(item.GetDict());
    if (!input_opt) {
      return absl::nullopt;
    }
    result.inputs_.push_back(std::move(*input_opt));
  }

  auto* outputs_list = value.FindList("outputs");
  if (!outputs_list) {
    return absl::nullopt;
  }
  for (auto& item : *outputs_list) {
    if (!item.is_dict()) {
      return absl::nullopt;
    }
    auto output_opt = ZCashTransaction::TxOutput::FromValue(item.GetDict());
    if (!output_opt) {
      return absl::nullopt;
    }
    result.outputs_.push_back(std::move(*output_opt));
  }

  if (!ReadUint32StringTo(value, "locktime", result.locktime_)) {
    return absl::nullopt;
  }

  if (!ReadStringTo(value, "to", result.to_)) {
    return absl::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount_)) {
    return absl::nullopt;
  }

  if (!ReadUint64StringTo(value, "fee", result.fee_)) {
    return absl::nullopt;
  }

  return result;
}

bool ZCashTransaction::IsSigned() const {
  if (inputs_.empty()) {
    return false;
  }

  return base::ranges::all_of(inputs_,
                              [](auto& input) { return input.IsSigned(); });
}

uint64_t ZCashTransaction::TotalInputsAmount() const {
  uint64_t result = 0;
  for (auto& input : inputs_) {
    result += input.utxo_value;
  }
  return result;
}

uint8_t ZCashTransaction::sighash_type() const {
  // We always sign all inputs.
  // return kZCashSigHashAll;
  return 0;
}

}  // namespace brave_wallet
