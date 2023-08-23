/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"

namespace brave_wallet {

namespace {

bool ReadStringTo(const base::Value::Dict& dict,
                  base::StringPiece key,
                  std::string& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  to = *str;
  return true;
}

bool ReadUint64StringTo(const base::Value::Dict& dict,
                        base::StringPiece key,
                        uint64_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint64(*str, &to);
}

bool ReadUint32StringTo(const base::Value::Dict& dict,
                        base::StringPiece key,
                        uint32_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint(*str, &to);
}

template <class T>
bool ReadDictTo(const base::Value::Dict& dict, base::StringPiece key, T& to) {
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
                        base::StringPiece key,
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

BitcoinTransaction::BitcoinTransaction() = default;
BitcoinTransaction::~BitcoinTransaction() = default;
BitcoinTransaction::BitcoinTransaction(BitcoinTransaction&& other) = default;
BitcoinTransaction& BitcoinTransaction::operator=(BitcoinTransaction&& other) =
    default;
bool BitcoinTransaction::operator==(const BitcoinTransaction& other) const {
  return std::tie(this->inputs_, this->outputs_, this->locktime_, this->to_,
                  this->amount_, this->fee_) ==
         std::tie(other.inputs_, other.outputs_, other.locktime_, other.to_,
                  other.amount_, other.fee_);
}
bool BitcoinTransaction::operator!=(const BitcoinTransaction& other) const {
  return !(*this == other);
}

BitcoinTransaction::Outpoint::Outpoint() = default;
BitcoinTransaction::Outpoint::~Outpoint() = default;
BitcoinTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
BitcoinTransaction::Outpoint& BitcoinTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
BitcoinTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
BitcoinTransaction::Outpoint& BitcoinTransaction::Outpoint::operator=(
    Outpoint&& other) = default;
bool BitcoinTransaction::Outpoint::operator==(
    const BitcoinTransaction::Outpoint& other) const {
  return std::tie(this->txid, this->index) == std::tie(other.txid, other.index);
}
bool BitcoinTransaction::Outpoint::operator!=(
    const BitcoinTransaction::Outpoint& other) const {
  return !(*this == other);
}

base::Value::Dict BitcoinTransaction::Outpoint::ToValue() const {
  base::Value::Dict dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", static_cast<int>(index));

  return dict;
}

// static
absl::optional<BitcoinTransaction::Outpoint>
BitcoinTransaction::Outpoint::FromValue(const base::Value::Dict& value) {
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

BitcoinTransaction::TxInput::TxInput() = default;
BitcoinTransaction::TxInput::~TxInput() = default;
BitcoinTransaction::TxInput::TxInput(BitcoinTransaction::TxInput&& other) =
    default;
BitcoinTransaction::TxInput& BitcoinTransaction::TxInput::operator=(
    BitcoinTransaction::TxInput&& other) = default;
bool BitcoinTransaction::TxInput::operator==(
    const BitcoinTransaction::TxInput& other) const {
  return std::tie(this->utxo_address, this->utxo_outpoint, this->utxo_value,
                  this->script_sig, this->witness) ==
         std::tie(other.utxo_address, other.utxo_outpoint, other.utxo_value,
                  other.script_sig, other.witness);
}
bool BitcoinTransaction::TxInput::operator!=(
    const BitcoinTransaction::TxInput& other) const {
  return !(*this == other);
}

BitcoinTransaction::TxInput BitcoinTransaction::TxInput::Clone() const {
  BitcoinTransaction::TxInput result;

  result.utxo_address = utxo_address;
  result.utxo_outpoint = utxo_outpoint;
  result.utxo_value = utxo_value;
  result.script_sig = script_sig;
  result.witness = witness;

  return result;
}

base::Value::Dict BitcoinTransaction::TxInput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("utxo_address", utxo_address);
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));

  dict.Set("script_sig", base::HexEncode(script_sig));
  dict.Set("witness", base::HexEncode(witness));

  return dict;
}

// static
absl::optional<BitcoinTransaction::TxInput>
BitcoinTransaction::TxInput::FromValue(const base::Value::Dict& value) {
  BitcoinTransaction::TxInput result;

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
absl::optional<BitcoinTransaction::TxInput>
BitcoinTransaction::TxInput::FromRpcUtxo(
    const std::string& address,
    const bitcoin_rpc::UnspentOutput& utxo) {
  BitcoinTransaction::TxInput result;
  result.utxo_address = address;

  if (!base::HexStringToSpan(utxo.txid, result.utxo_outpoint.txid)) {
    return absl::nullopt;
  }
  if (!base::StringToUint(utxo.vout, &result.utxo_outpoint.index)) {
    return absl::nullopt;
  }
  if (!base::StringToUint64(utxo.value, &result.utxo_value)) {
    return absl::nullopt;
  }

  return result;
}

uint32_t BitcoinTransaction::TxInput::n_sequence() const {
  // Fixed value by now.
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L945
  return 0xfffffffd;
}

bool BitcoinTransaction::TxInput::IsSigned() const {
  return !script_sig.empty() || !witness.empty();
}

BitcoinTransaction::TxOutput::TxOutput() = default;
BitcoinTransaction::TxOutput::~TxOutput() = default;
BitcoinTransaction::TxOutput::TxOutput(BitcoinTransaction::TxOutput&& other) =
    default;
BitcoinTransaction::TxOutput& BitcoinTransaction::TxOutput::operator=(
    BitcoinTransaction::TxOutput&& other) = default;
bool BitcoinTransaction::TxOutput::operator==(
    const BitcoinTransaction::TxOutput& other) const {
  return std::tie(this->address, this->amount) ==
         std::tie(other.address, other.amount);
}
bool BitcoinTransaction::TxOutput::operator!=(
    const BitcoinTransaction::TxOutput& other) const {
  return !(*this == other);
}

BitcoinTransaction::TxOutput BitcoinTransaction::TxOutput::Clone() const {
  BitcoinTransaction::TxOutput result;

  result.address = address;
  result.amount = amount;

  return result;
}

base::Value::Dict BitcoinTransaction::TxOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("address", address);
  dict.Set("amount", base::NumberToString(amount));

  return dict;
}

// static
absl::optional<BitcoinTransaction::TxOutput>
BitcoinTransaction::TxOutput::FromValue(const base::Value::Dict& value) {
  BitcoinTransaction::TxOutput result;
  if (!ReadStringTo(value, "address", result.address)) {
    return absl::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return absl::nullopt;
  }

  return result;
}

BitcoinTransaction BitcoinTransaction::Clone() const {
  BitcoinTransaction result;

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

base::Value::Dict BitcoinTransaction::ToValue() const {
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
absl::optional<BitcoinTransaction> BitcoinTransaction::FromValue(
    const base::Value::Dict& value) {
  BitcoinTransaction result;

  auto* inputs_list = value.FindList("inputs");
  if (!inputs_list) {
    return absl::nullopt;
  }
  for (auto& item : *inputs_list) {
    if (!item.is_dict()) {
      return absl::nullopt;
    }
    auto input_opt = BitcoinTransaction::TxInput::FromValue(item.GetDict());
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
    auto output_opt = BitcoinTransaction::TxOutput::FromValue(item.GetDict());
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

bool BitcoinTransaction::IsSigned() const {
  if (inputs_.empty()) {
    return false;
  }

  return base::ranges::all_of(inputs_,
                              [](auto& input) { return input.IsSigned(); });
}

uint64_t BitcoinTransaction::TotalInputsAmount() const {
  uint64_t result = 0;
  for (auto& input : inputs_) {
    result += input.utxo_value;
  }
  return result;
}

uint8_t BitcoinTransaction::sighash_type() const {
  // We always sign all inputs.
  return kBitcoinSigHashAll;
}

}  // namespace brave_wallet
