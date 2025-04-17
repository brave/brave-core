/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/numerics/clamped_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/optional_util.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

namespace {

constexpr char kChangeOuputType[] = "change";
constexpr char kTargetOutputType[] = "target";

std::optional<std::string> ReadString(const base::Value::Dict& dict,
                                      std::string_view key) {
  return base::OptionalFromPtr(dict.FindString(key));
}

std::optional<CardanoAddress> ReadCardanoAddress(const base::Value::Dict& dict,
                                                 std::string_view key) {
  auto address_string = ReadString(dict, key);
  if (!address_string) {
    return std::nullopt;
  }

  return CardanoAddress::FromString(*address_string);
}

std::optional<uint64_t> ReadUint64String(const base::Value::Dict& dict,
                                         std::string_view key) {
  auto* str = dict.FindString(key);
  if (!str) {
    return std::nullopt;
  }

  uint64_t result = 0;
  if (!base::StringToUint64(*str, &result)) {
    return std::nullopt;
  }
  return result;
}

std::optional<uint32_t> ReadUint32String(const base::Value::Dict& dict,
                                         std::string_view key) {
  auto* str = dict.FindString(key);
  if (!str) {
    return std::nullopt;
  }

  uint32_t result = 0;
  if (!base::StringToUint(*str, &result)) {
    return std::nullopt;
  }
  return result;
}

template <class T>
std::optional<T> ReadDict(const base::Value::Dict& dict, std::string_view key) {
  auto* key_dict = dict.FindDict(key);
  if (!key_dict) {
    return std::nullopt;
  }
  return T::FromValue(*key_dict);
}

template <size_t SZ>
std::optional<std::array<uint8_t, SZ>> ReadHexByteArray(
    const base::Value::Dict& dict,
    std::string_view key) {
  auto* str = dict.FindString(key);
  if (!str) {
    return std::nullopt;
  }
  std::array<uint8_t, SZ> result = {};
  if (!base::HexStringToSpan(*str, result)) {
    return std::nullopt;
  }
  return result;
}

}  // namespace

CardanoTransaction::CardanoTransaction() = default;
CardanoTransaction::~CardanoTransaction() = default;
CardanoTransaction::CardanoTransaction(const CardanoTransaction& other) =
    default;
CardanoTransaction& CardanoTransaction::operator=(
    const CardanoTransaction& other) = default;
CardanoTransaction::CardanoTransaction(CardanoTransaction&& other) = default;
CardanoTransaction& CardanoTransaction::operator=(CardanoTransaction&& other) =
    default;
bool CardanoTransaction::operator==(const CardanoTransaction& other) const =
    default;
bool CardanoTransaction::operator!=(const CardanoTransaction& other) const =
    default;

CardanoTransaction::Outpoint::Outpoint() = default;
CardanoTransaction::Outpoint::~Outpoint() = default;
CardanoTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
CardanoTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    Outpoint&& other) = default;

base::Value::Dict CardanoTransaction::Outpoint::ToValue() const {
  base::Value::Dict dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", static_cast<int>(index));

  return dict;
}

// static
std::optional<CardanoTransaction::Outpoint>
CardanoTransaction::Outpoint::FromValue(const base::Value::Dict& value) {
  Outpoint result;

  auto* txid_hex = value.FindString("txid");
  if (!txid_hex) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(*txid_hex, result.txid)) {
    return std::nullopt;
  }

  auto index_value = value.FindInt("index");
  if (!index_value) {
    return std::nullopt;
  }
  result.index = *index_value;

  return result;
}

CardanoTransaction::TxInput::TxInput() = default;
CardanoTransaction::TxInput::~TxInput() = default;
CardanoTransaction::TxInput::TxInput(const CardanoTransaction::TxInput& other) =
    default;
CardanoTransaction::TxInput& CardanoTransaction::TxInput::operator=(
    const CardanoTransaction::TxInput& other) = default;
CardanoTransaction::TxInput::TxInput(CardanoTransaction::TxInput&& other) =
    default;
CardanoTransaction::TxInput& CardanoTransaction::TxInput::operator=(
    CardanoTransaction::TxInput&& other) = default;

base::Value::Dict CardanoTransaction::TxInput::ToValue() const {
  base::Value::Dict dict;

  // TODO(https://github.com/brave/brave-browser/issues/45411): implement with
  // json_schema_compiler.
  dict.Set("utxo_address", utxo_address.ToString());
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));

  return dict;
}

// static
std::optional<CardanoTransaction::TxInput>
CardanoTransaction::TxInput::FromValue(const base::Value::Dict& value) {
  CardanoTransaction::TxInput result;

  if (!base::OptionalUnwrapTo(ReadCardanoAddress(value, "utxo_address"),
                              result.utxo_address)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadDict<Outpoint>(value, "utxo_outpoint"),
                              result.utxo_outpoint)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadUint64String(value, "utxo_value"),
                              result.utxo_value)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<CardanoTransaction::TxInput>
CardanoTransaction::TxInput::FromRpcUtxo(
    const CardanoAddress& address,
    const cardano_rpc::UnspentOutput& utxo) {
  CardanoTransaction::TxInput result;

  result.utxo_address = address;
  result.utxo_outpoint.txid = utxo.tx_hash;
  result.utxo_outpoint.index = utxo.output_index;
  result.utxo_value = utxo.lovelace_amount;

  return result;
}

CardanoTransaction::TxWitness::TxWitness() = default;
CardanoTransaction::TxWitness::TxWitness(
    std::array<uint8_t, kCardanoWitnessSize> witness_bytes)
    : witness_bytes(witness_bytes) {}
CardanoTransaction::TxWitness::~TxWitness() = default;
CardanoTransaction::TxWitness::TxWitness(
    const CardanoTransaction::TxWitness& other) = default;
CardanoTransaction::TxWitness& CardanoTransaction::TxWitness::operator=(
    const CardanoTransaction::TxWitness& other) = default;
CardanoTransaction::TxWitness::TxWitness(
    CardanoTransaction::TxWitness&& other) = default;
CardanoTransaction::TxWitness& CardanoTransaction::TxWitness::operator=(
    CardanoTransaction::TxWitness&& other) = default;

base::Value::Dict CardanoTransaction::TxWitness::ToValue() const {
  base::Value::Dict dict;

  dict.Set("witness_bytes", base::HexEncode(witness_bytes));

  return dict;
}

// static
std::optional<CardanoTransaction::TxWitness>
CardanoTransaction::TxWitness::FromValue(const base::Value::Dict& value) {
  CardanoTransaction::TxWitness result;

  if (!base::OptionalUnwrapTo(
          ReadHexByteArray<kCardanoWitnessSize>(value, "witness_bytes"),
          result.witness_bytes)) {
    return std::nullopt;
  }

  return result;
}

// static
CardanoTransaction::TxWitness CardanoTransaction::TxWitness::DummyTxWitness() {
  return CardanoTransaction::TxWitness();
}

CardanoTransaction::TxOutput::TxOutput() = default;
CardanoTransaction::TxOutput::~TxOutput() = default;
CardanoTransaction::TxOutput::TxOutput(
    const CardanoTransaction::TxOutput& other) = default;
CardanoTransaction::TxOutput& CardanoTransaction::TxOutput::operator=(
    const CardanoTransaction::TxOutput& other) = default;
CardanoTransaction::TxOutput::TxOutput(CardanoTransaction::TxOutput&& other) =
    default;
CardanoTransaction::TxOutput& CardanoTransaction::TxOutput::operator=(
    CardanoTransaction::TxOutput&& other) = default;

base::Value::Dict CardanoTransaction::TxOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("type", type == TxOutputType::kTarget ? kTargetOutputType
                                                 : kChangeOuputType);
  dict.Set("address", address.ToString());
  dict.Set("amount", base::NumberToString(amount));

  return dict;
}

// static
std::optional<CardanoTransaction::TxOutput>
CardanoTransaction::TxOutput::FromValue(const base::Value::Dict& value) {
  CardanoTransaction::TxOutput result;

  if (auto type = ReadString(value, "type")) {
    if (*type != kChangeOuputType && *type != kTargetOutputType) {
      return std::nullopt;
    }
    result.type = *type == kTargetOutputType ? TxOutputType::kTarget
                                             : TxOutputType::kChange;
  } else {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadCardanoAddress(value, "address"),
                              result.address)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadUint64String(value, "amount"),
                              result.amount)) {
    return std::nullopt;
  }

  return result;
}

base::Value::Dict CardanoTransaction::ToValue() const {
  base::Value::Dict dict;

  auto& inputs_value = dict.Set("inputs", base::Value::List())->GetList();
  for (const auto& input : inputs_) {
    inputs_value.Append(input.ToValue());
  }

  auto& outputs_value = dict.Set("outputs", base::Value::List())->GetList();
  for (const auto& output : outputs_) {
    outputs_value.Append(output.ToValue());
  }

  dict.Set("invalid_after", base::NumberToString(invalid_after_));
  dict.Set("to", to_.ToString());
  dict.Set("amount", base::NumberToString(amount_));
  dict.Set("sending_max_amount", sending_max_amount_);

  return dict;
}

// static
std::optional<CardanoTransaction> CardanoTransaction::FromValue(
    const base::Value::Dict& value) {
  CardanoTransaction result;

  auto* inputs_list = value.FindList("inputs");
  if (!inputs_list) {
    return std::nullopt;
  }
  for (const auto& item : *inputs_list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }
    auto input_opt = CardanoTransaction::TxInput::FromValue(item.GetDict());
    if (!input_opt) {
      return std::nullopt;
    }
    result.inputs_.push_back(std::move(*input_opt));
  }

  auto* outputs_list = value.FindList("outputs");
  if (!outputs_list) {
    return std::nullopt;
  }
  for (const auto& item : *outputs_list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }
    auto output_opt = CardanoTransaction::TxOutput::FromValue(item.GetDict());
    if (!output_opt) {
      return std::nullopt;
    }
    result.outputs_.push_back(std::move(*output_opt));
  }

  if (!base::OptionalUnwrapTo(ReadUint32String(value, "invalid_after"),
                              result.invalid_after_)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadCardanoAddress(value, "to"), result.to_)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadUint64String(value, "amount"),
                              result.amount_)) {
    return std::nullopt;
  }

  result.sending_max_amount_ =
      value.FindBool("sending_max_amount").value_or(false);

  return result;
}

bool CardanoTransaction::IsSigned() const {
  if (inputs_.empty()) {
    return false;
  }

  return inputs_.size() == witnesses_.size();
}

uint64_t CardanoTransaction::TotalInputsAmount() const {
  uint64_t result = 0;
  for (const auto& input : inputs_) {
    result += input.utxo_value;
  }
  return result;
}

uint64_t CardanoTransaction::TotalOutputsAmount() const {
  uint64_t result = 0;
  for (const auto& output : outputs_) {
    result += output.amount;
  }
  return result;
}

bool CardanoTransaction::AmountsAreValid(uint64_t min_fee) const {
  return TotalInputsAmount() >= TotalOutputsAmount() + min_fee;
}

uint64_t CardanoTransaction::EffectiveFeeAmount() const {
  return base::ClampSub(TotalInputsAmount(), TotalOutputsAmount());
}

void CardanoTransaction::AddInput(TxInput input) {
  inputs_.push_back(std::move(input));
}

void CardanoTransaction::AddInputs(std::vector<TxInput> inputs) {
  for (auto input : inputs) {
    inputs_.push_back(std::move(input));
  }
}

void CardanoTransaction::ClearInputs() {
  inputs_.clear();
}

void CardanoTransaction::AddWitness(TxWitness witnesses) {
  witnesses_.push_back(std::move(witnesses));
}

void CardanoTransaction::SetWitnesses(std::vector<TxWitness> witnesses) {
  witnesses_ = std::move(witnesses);
}

void CardanoTransaction::AddOutput(TxOutput output) {
  outputs_.push_back(std::move(output));
}

void CardanoTransaction::ClearOutputs() {
  outputs_.clear();
}

void CardanoTransaction::ClearChangeOutput() {
  std::erase_if(outputs_, [](auto& output) {
    return output.type == CardanoTransaction::TxOutputType::kChange;
  });
}

const CardanoTransaction::TxOutput* CardanoTransaction::TargetOutput() const {
  for (const auto& output : outputs_) {
    if (output.type == CardanoTransaction::TxOutputType::kTarget) {
      return &output;
    }
  }
  return nullptr;
}

const CardanoTransaction::TxOutput* CardanoTransaction::ChangeOutput() const {
  for (const auto& output : outputs_) {
    if (output.type == CardanoTransaction::TxOutputType::kChange) {
      return &output;
    }
  }
  return nullptr;
}

CardanoTransaction::TxOutput* CardanoTransaction::TargetOutput() {
  for (auto& output : outputs_) {
    if (output.type == CardanoTransaction::TxOutputType::kTarget) {
      return &output;
    }
  }
  return nullptr;
}

CardanoTransaction::TxOutput* CardanoTransaction::ChangeOutput() {
  for (auto& output : outputs_) {
    if (output.type == CardanoTransaction::TxOutputType::kChange) {
      return &output;
    }
  }
  return nullptr;
}

uint64_t CardanoTransaction::MoveSurplusFeeToChangeOutput(uint64_t min_fee) {
  auto* change = ChangeOutput();
  if (!change) {
    return 0;
  }
  auto* target = TargetOutput();
  CHECK(target);

  auto total_input = TotalInputsAmount();
  if (total_input > min_fee + target->amount) {
    DCHECK_EQ(change->amount, 0u);
    change->amount = total_input - (min_fee + target->amount);
    DCHECK_EQ(EffectiveFeeAmount(), min_fee);
    return change->amount;
  }

  return 0;
}

}  // namespace brave_wallet
