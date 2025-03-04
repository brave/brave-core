/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_utils.h"

namespace brave_wallet {

namespace {

constexpr char kChangeOuputType[] = "change";
constexpr char kTargetOutputType[] = "target";

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

bool ReadOptionalHexByteArrayTo(const base::Value::Dict& dict,
                                std::string_view key,
                                std::optional<std::vector<uint8_t>>& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return true;
  }
  if (str->empty()) {
    to.reset();
    return true;
  }
  to.emplace();
  return base::HexStringToBytes(*str, &to.value());
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
bool CardanoTransaction::operator==(const CardanoTransaction& other) const {
  return std::tie(this->inputs_, this->outputs_, this->locktime_, this->to_,
                  this->amount_) == std::tie(other.inputs_, other.outputs_,
                                             other.locktime_, other.to_,
                                             other.amount_);
}
bool CardanoTransaction::operator!=(const CardanoTransaction& other) const {
  return !(*this == other);
}

CardanoTransaction::Outpoint::Outpoint() = default;
CardanoTransaction::Outpoint::~Outpoint() = default;
CardanoTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
CardanoTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    Outpoint&& other) = default;
bool CardanoTransaction::Outpoint::operator==(
    const CardanoTransaction::Outpoint& other) const {
  return std::tie(this->txid, this->index) == std::tie(other.txid, other.index);
}
bool CardanoTransaction::Outpoint::operator!=(
    const CardanoTransaction::Outpoint& other) const {
  return !(*this == other);
}
bool CardanoTransaction::Outpoint::operator<(
    const CardanoTransaction::Outpoint& other) const {
  return std::tie(this->txid, this->index) < std::tie(other.txid, other.index);
}

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
bool CardanoTransaction::TxInput::operator==(
    const CardanoTransaction::TxInput& other) const {
  return std::tie(this->utxo_address, this->utxo_outpoint, this->utxo_value,
                  this->script_sig, this->witness, this->raw_outpoint_tx) ==
         std::tie(other.utxo_address, other.utxo_outpoint, other.utxo_value,
                  other.script_sig, other.witness, other.raw_outpoint_tx);
}
bool CardanoTransaction::TxInput::operator!=(
    const CardanoTransaction::TxInput& other) const {
  return !(*this == other);
}

base::Value::Dict CardanoTransaction::TxInput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("utxo_address", utxo_address);
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));
  if (raw_outpoint_tx) {
    dict.Set("raw_outpoint_tx", base::HexEncode(raw_outpoint_tx.value()));
  }

  dict.Set("script_sig", base::HexEncode(script_sig));
  dict.Set("witness", base::HexEncode(witness));

  return dict;
}

// static
std::optional<CardanoTransaction::TxInput>
CardanoTransaction::TxInput::FromValue(const base::Value::Dict& value) {
  CardanoTransaction::TxInput result;

  if (!ReadStringTo(value, "utxo_address", result.utxo_address)) {
    return std::nullopt;
  }

  if (!ReadDictTo(value, "utxo_outpoint", result.utxo_outpoint)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "utxo_value", result.utxo_value)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_sig", result.script_sig)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "witness", result.witness)) {
    return std::nullopt;
  }

  if (!ReadOptionalHexByteArrayTo(value, "raw_outpoint_tx",
                                  result.raw_outpoint_tx)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<CardanoTransaction::TxInput>
CardanoTransaction::TxInput::FromRpcUtxo(
    const std::string& address,
    const cardano_rpc::UnspentOutput& utxo) {
  CardanoTransaction::TxInput result;
  result.utxo_address = address;

  if (!base::HexStringToSpan(utxo.tx_hash, result.utxo_outpoint.txid)) {
    return std::nullopt;
  }
  if (!base::StringToUint(utxo.output_index, &result.utxo_outpoint.index)) {
    return std::nullopt;
  }

  if (auto lovelace_amount = GetLovelaceAmountFromUtxo(utxo)) {
    result.utxo_value = *lovelace_amount;
  } else {
    return std::nullopt;
  }

  return result;
}

uint32_t CardanoTransaction::TxInput::n_sequence() const {
  // Fixed value by now.
  // https://github.com/cardano/cardano/blob/v24.0/src/wallet/spend.cpp#L945
  return 0xfffffffd;
}

bool CardanoTransaction::TxInput::IsSigned() const {
  return !script_sig.empty() || !witness.empty();
}

CardanoTransaction::TxInputGroup::TxInputGroup() = default;
CardanoTransaction::TxInputGroup::~TxInputGroup() = default;
CardanoTransaction::TxInputGroup::TxInputGroup(
    const CardanoTransaction::TxInputGroup& other) = default;
CardanoTransaction::TxInputGroup& CardanoTransaction::TxInputGroup::operator=(
    const CardanoTransaction::TxInputGroup& other) = default;
CardanoTransaction::TxInputGroup::TxInputGroup(
    CardanoTransaction::TxInputGroup&& other) = default;
CardanoTransaction::TxInputGroup& CardanoTransaction::TxInputGroup::operator=(
    CardanoTransaction::TxInputGroup&& other) = default;

void CardanoTransaction::TxInputGroup::AddInput(
    CardanoTransaction::TxInput input) {
  total_amount_ += input.utxo_value;
  inputs_.push_back(std::move(input));
}

void CardanoTransaction::TxInputGroup::AddInputs(
    std::vector<CardanoTransaction::TxInput> inputs) {
  for (auto& input : inputs_) {
    total_amount_ += input.utxo_value;
    inputs_.push_back(std::move(input));
  }
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
bool CardanoTransaction::TxOutput::operator==(
    const CardanoTransaction::TxOutput& other) const {
  return std::tie(this->type, this->address, this->script_pubkey,
                  this->amount) ==
         std::tie(other.type, other.address, other.script_pubkey, other.amount);
}
bool CardanoTransaction::TxOutput::operator!=(
    const CardanoTransaction::TxOutput& other) const {
  return !(*this == other);
}

base::Value::Dict CardanoTransaction::TxOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("type", type == TxOutputType::kTarget ? kTargetOutputType
                                                 : kChangeOuputType);
  dict.Set("address", address);
  dict.Set("script_pubkey", base::HexEncode(script_pubkey));
  dict.Set("amount", base::NumberToString(amount));

  return dict;
}

// static
std::optional<CardanoTransaction::TxOutput>
CardanoTransaction::TxOutput::FromValue(const base::Value::Dict& value) {
  CardanoTransaction::TxOutput result;

  std::string type_string;
  if (!ReadStringTo(value, "type", type_string) &&
      type_string != kChangeOuputType && type_string != kTargetOutputType) {
    return std::nullopt;
  }
  result.type = type_string == kTargetOutputType ? TxOutputType::kTarget
                                                 : TxOutputType::kChange;

  if (!ReadStringTo(value, "address", result.address)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_pubkey", result.script_pubkey)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return std::nullopt;
  }

  return result;
}

base::Value::Dict CardanoTransaction::ToValue() const {
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
  for (auto& item : *inputs_list) {
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
  for (auto& item : *outputs_list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }
    auto output_opt = CardanoTransaction::TxOutput::FromValue(item.GetDict());
    if (!output_opt) {
      return std::nullopt;
    }
    result.outputs_.push_back(std::move(*output_opt));
  }

  if (!ReadUint32StringTo(value, "locktime", result.locktime_)) {
    return std::nullopt;
  }

  if (!ReadStringTo(value, "to", result.to_)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount_)) {
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

  return std::ranges::all_of(inputs_,
                             [](auto& input) { return input.IsSigned(); });
}

uint64_t CardanoTransaction::TotalInputsAmount() const {
  uint64_t result = 0;
  for (auto& input : inputs_) {
    result += input.utxo_value;
  }
  return result;
}

uint64_t CardanoTransaction::TotalOutputsAmount() const {
  uint64_t result = 0;
  for (auto& output : outputs_) {
    result += output.amount;
  }
  return result;
}

bool CardanoTransaction::AmountsAreValid(uint64_t min_fee) const {
  return TotalInputsAmount() >= TotalOutputsAmount() + min_fee;
}

uint64_t CardanoTransaction::EffectiveFeeAmount() const {
  DCHECK_GE(TotalInputsAmount(), TotalOutputsAmount());
  return TotalInputsAmount() - TotalOutputsAmount();
}

void CardanoTransaction::AddInput(TxInput input) {
  inputs_.push_back(std::move(input));
}

void CardanoTransaction::AddInputs(std::vector<TxInput> inputs) {
  for (auto& input : inputs) {
    inputs_.push_back(std::move(input));
  }
}

void CardanoTransaction::ClearInputs() {
  inputs_.clear();
}

void CardanoTransaction::SetInputWitness(size_t input_index,
                                         std::vector<uint8_t> witness) {
  CHECK_LT(input_index, inputs_.size());
  inputs_[input_index].witness = std::move(witness);
}

void CardanoTransaction::SetInputRawTransaction(size_t input_index,
                                                std::vector<uint8_t> raw_tx) {
  CHECK_LT(input_index, inputs_.size());
  inputs_[input_index].raw_outpoint_tx = std::move(raw_tx);
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
  for (auto& output : outputs_) {
    if (output.type == CardanoTransaction::TxOutputType::kTarget) {
      return &output;
    }
  }
  return nullptr;
}

const CardanoTransaction::TxOutput* CardanoTransaction::ChangeOutput() const {
  for (auto& output : outputs_) {
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

void CardanoTransaction::ShuffleTransaction() {
  base::RandomShuffle(inputs_.begin(), inputs_.end());
  base::RandomShuffle(outputs_.begin(), outputs_.end());
}

void CardanoTransaction::ArrangeTransactionForTesting() {
  std::sort(inputs_.begin(), inputs_.end(),
            [](const auto& input1, const auto& input2) {
              return input1.utxo_outpoint < input2.utxo_outpoint;
            });

  DCHECK_LE(outputs_.size(), 2u);
  std::sort(outputs_.begin(), outputs_.end(),
            [](const auto& output1, const auto& output2) {
              return output1.type < output2.type;
            });
}

}  // namespace brave_wallet
