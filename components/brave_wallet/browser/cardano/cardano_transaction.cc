/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/optional_util.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

namespace {

constexpr char kChangeOuputType[] = "change";
constexpr char kTargetOutputType[] = "target";

std::optional<std::string> ReadString(const base::DictValue& dict,
                                      std::string_view key) {
  return base::OptionalFromPtr(dict.FindString(key));
}

std::optional<CardanoAddress> ReadCardanoAddress(const base::DictValue& dict,
                                                 std::string_view key) {
  auto address_string = ReadString(dict, key);
  if (!address_string) {
    return std::nullopt;
  }

  return CardanoAddress::FromString(*address_string);
}

std::optional<uint64_t> ReadUint64String(const base::DictValue& dict,
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

template <class T>
std::optional<T> ReadDict(const base::DictValue& dict, std::string_view key) {
  auto* key_dict = dict.FindDict(key);
  if (!key_dict) {
    return std::nullopt;
  }
  return T::FromValue(*key_dict);
}

template <size_t SZ>
std::optional<std::array<uint8_t, SZ>> ReadHexByteArray(
    const base::DictValue& dict,
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

std::optional<std::vector<uint8_t>> ReadHexByteVector(
    const base::DictValue& dict,
    std::string_view key) {
  auto* str = dict.FindString(key);
  if (!str) {
    return std::nullopt;
  }
  std::vector<uint8_t> result = {};
  if (!base::HexStringToBytes(*str, &result)) {
    return std::nullopt;
  }
  return result;
}

base::ListValue TokensToValue(const cardano_rpc::Tokens& tokens) {
  base::ListValue result;
  for (auto& token : tokens) {
    base::DictValue token_value;
    token_value.Set("token_id", base::HexEncode(token.first));
    token_value.Set("amount", base::NumberToString(token.second));
    result.Append(std::move(token_value));
  }
  return result;
}

std::optional<cardano_rpc::Tokens> TokensFromValue(
    const base::ListValue* tokens_list) {
  if (!tokens_list) {
    return std::nullopt;
  }

  cardano_rpc::Tokens result;

  for (const auto& item : *tokens_list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }

    const base::DictValue& token_value = item.GetDict();

    cardano_rpc::TokenId token_id;

    if (!base::OptionalUnwrapTo(ReadHexByteVector(token_value, "token_id"),
                                token_id)) {
      return std::nullopt;
    }
    // Fixed size policy id and non-empty name.
    if (token_id.size() < kCardanoScriptHashSize + 1u) {
      return std::nullopt;
    }

    uint64_t token_amount = 0;
    if (!base::OptionalUnwrapTo(ReadUint64String(token_value, "amount"),
                                token_amount)) {
      return std::nullopt;
    }
    result[token_id] = token_amount;
  }

  return result;
}

// For each token do `from[token] -= tokens[token]` ensuring no overflows.
bool SubtractTokens(cardano_rpc::Tokens& from,
                    const cardano_rpc::Tokens& tokens) {
  for (auto& token : tokens) {
    auto from_token = from.find(token);
    if (from_token == from.end()) {
      return false;
    }
    if (!base::CheckSub<uint64_t>(from_token->second, token.second)
             .AssignIfValid(&from_token->second)) {
      return false;
    }
    if (from_token->second == 0u) {
      from.erase(from_token);
    }
  }

  return true;
}

}  // namespace

TxBuilderParms::TxBuilderParms() = default;
TxBuilderParms::~TxBuilderParms() = default;
TxBuilderParms::TxBuilderParms(const TxBuilderParms&) = default;
TxBuilderParms& TxBuilderParms::operator=(const TxBuilderParms&) = default;
TxBuilderParms::TxBuilderParms(TxBuilderParms&&) = default;
TxBuilderParms& TxBuilderParms::operator=(TxBuilderParms&&) = default;

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

CardanoTransaction::Outpoint::Outpoint() = default;
CardanoTransaction::Outpoint::~Outpoint() = default;
CardanoTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
CardanoTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
CardanoTransaction::Outpoint& CardanoTransaction::Outpoint::operator=(
    Outpoint&& other) = default;

base::DictValue CardanoTransaction::Outpoint::ToValue() const {
  base::DictValue dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", base::checked_cast<int>(index));

  return dict;
}

// static
std::optional<CardanoTransaction::Outpoint>
CardanoTransaction::Outpoint::FromValue(const base::DictValue& value) {
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

base::DictValue CardanoTransaction::TxInput::ToValue() const {
  base::DictValue dict;

  // TODO(https://github.com/brave/brave-browser/issues/45411): implement with
  // json_schema_compiler.
  dict.Set("utxo_address", utxo_address.ToString());
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));
  dict.Set("utxo_tokens", TokensToValue(utxo_tokens));

  return dict;
}

// static
std::optional<CardanoTransaction::TxInput>
CardanoTransaction::TxInput::FromValue(const base::DictValue& value) {
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

  if (!base::OptionalUnwrapTo(TokensFromValue(value.FindList("utxo_tokens")),
                              result.utxo_tokens)) {
    return std::nullopt;
  }

  return result;
}

// static
CardanoTransaction::TxInput CardanoTransaction::TxInput::FromRpcUtxo(
    const cardano_rpc::UnspentOutput& utxo) {
  CardanoTransaction::TxInput result;

  result.utxo_address = utxo.address_to;
  result.utxo_outpoint.txid = utxo.tx_hash;
  result.utxo_outpoint.index = utxo.output_index;
  result.utxo_value = utxo.lovelace_amount;
  result.utxo_tokens = utxo.tokens;

  return result;
}

CardanoTransaction::TxWitness::TxWitness() = default;
CardanoTransaction::TxWitness::TxWitness(
    std::array<uint8_t, kCardanoPubKeySize> public_key,
    std::array<uint8_t, kCardanoSignatureSize> signature)
    : public_key(public_key), signature(signature) {}
CardanoTransaction::TxWitness::~TxWitness() = default;
CardanoTransaction::TxWitness::TxWitness(
    const CardanoTransaction::TxWitness& other) = default;
CardanoTransaction::TxWitness& CardanoTransaction::TxWitness::operator=(
    const CardanoTransaction::TxWitness& other) = default;
CardanoTransaction::TxWitness::TxWitness(
    CardanoTransaction::TxWitness&& other) = default;
CardanoTransaction::TxWitness& CardanoTransaction::TxWitness::operator=(
    CardanoTransaction::TxWitness&& other) = default;

base::DictValue CardanoTransaction::TxWitness::ToValue() const {
  base::DictValue dict;

  dict.Set("public_key", base::HexEncode(public_key));
  dict.Set("signature", base::HexEncode(signature));

  return dict;
}

// static
std::optional<CardanoTransaction::TxWitness>
CardanoTransaction::TxWitness::FromValue(const base::DictValue& value) {
  CardanoTransaction::TxWitness result;

  // Try to read from legacy witness_bytes first.
  std::array<uint8_t, kCardanoPubKeySize + kCardanoSignatureSize>
      witness_bytes = {};
  if (base::OptionalUnwrapTo(
          ReadHexByteArray<kCardanoPubKeySize + kCardanoSignatureSize>(
              value, "witness_bytes"),
          witness_bytes)) {
    base::span(result.public_key)
        .copy_from_nonoverlapping(
            base::span(witness_bytes).first<kCardanoPubKeySize>());
    base::span(result.signature)
        .copy_from_nonoverlapping(
            base::span(witness_bytes).last<kCardanoSignatureSize>());

    return result;
  }

  if (!base::OptionalUnwrapTo(
          ReadHexByteArray<kCardanoPubKeySize>(value, "public_key"),
          result.public_key)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(
          ReadHexByteArray<kCardanoSignatureSize>(value, "signature"),
          result.signature)) {
    return std::nullopt;
  }

  return result;
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

base::DictValue CardanoTransaction::TxOutput::ToValue() const {
  base::DictValue dict;

  dict.Set("type", type == TxOutputType::kTarget ? kTargetOutputType
                                                 : kChangeOuputType);
  dict.Set("address", address.ToString());
  dict.Set("amount", base::NumberToString(amount));
  dict.Set("tokens", TokensToValue(tokens));

  return dict;
}

// static
std::optional<CardanoTransaction::TxOutput>
CardanoTransaction::TxOutput::FromValue(const base::DictValue& value) {
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

  if (!base::OptionalUnwrapTo(TokensFromValue(value.FindList("tokens")),
                              result.tokens)) {
    return std::nullopt;
  }

  return result;
}

CardanoTxDecoder::SerializableTxOutput
CardanoTransaction::TxOutput::ToSerializableTxOutput() const {
  CardanoTxDecoder::SerializableTxOutput result;

  result.address_bytes = address.ToCborBytes();
  result.amount = amount;

  result.tokens.reserve(tokens.size());
  for (const auto& token : tokens) {
    auto& token_result = result.tokens.emplace_back();
    token_result.token_id = token.first;
    token_result.amount = token.second;
  }

  return result;
}

base::DictValue CardanoTransaction::ToValue() const {
  base::DictValue dict;

  auto& inputs_value = dict.Set("inputs", base::ListValue())->GetList();
  for (const auto& input : inputs_) {
    inputs_value.Append(input.ToValue());
  }

  auto& outputs_value = dict.Set("outputs", base::ListValue())->GetList();
  for (const auto& output : outputs_) {
    outputs_value.Append(output.ToValue());
  }

  auto& witnesses_value = dict.Set("witnesses", base::ListValue())->GetList();
  for (const auto& witness : witnesses_) {
    witnesses_value.Append(witness.ToValue());
  }

  dict.Set("invalid_after", base::NumberToString(invalid_after_));
  dict.Set("fee", base::NumberToString(fee_));

  return dict;
}

// static
std::optional<CardanoTransaction> CardanoTransaction::FromValue(
    const base::DictValue& value) {
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

  auto* witnesses_list = value.FindList("witnesses");
  if (!witnesses_list) {
    return std::nullopt;
  }
  for (const auto& item : *witnesses_list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }
    auto output_opt = CardanoTransaction::TxWitness::FromValue(item.GetDict());
    if (!output_opt) {
      return std::nullopt;
    }
    result.witnesses_.push_back(std::move(*output_opt));
  }

  if (!base::OptionalUnwrapTo(ReadUint64String(value, "invalid_after"),
                              result.invalid_after_)) {
    return std::nullopt;
  }

  if (!base::OptionalUnwrapTo(ReadUint64String(value, "fee"), result.fee_)) {
    base::CheckedNumeric<uint64_t> fee =
        result.GetTotalInputsAmount() - result.GetTotalOutputsAmount();
    if (!fee.AssignIfValid(&result.fee_)) {
      return std::nullopt;
    }
  }

  return result;
}

void CardanoTransaction::SetupTargetOutput(CardanoAddress target_address) {
  CHECK(!TargetOutput());
  CardanoTransaction::TxOutput target_output;
  target_output.type = CardanoTransaction::TxOutputType::kTarget;
  target_output.amount = 0;
  target_output.address = std::move(target_address);

  AddOutput(std::move(target_output));
}

void CardanoTransaction::SetupChangeOutput(CardanoAddress change_address) {
  CHECK(!ChangeOutput());
  CardanoTransaction::TxOutput change_output;
  change_output.type = CardanoTransaction::TxOutputType::kChange;
  change_output.amount = 0;
  change_output.address = std::move(change_address);

  AddOutput(std::move(change_output));
}

base::CheckedNumeric<uint64_t> CardanoTransaction::GetTotalInputsAmount()
    const {
  base::CheckedNumeric<uint64_t> result = 0;
  for (const auto& input : inputs_) {
    result += input.utxo_value;
  }
  return result;
}

base::CheckedNumeric<uint64_t> CardanoTransaction::GetTotalOutputsAmount()
    const {
  base::CheckedNumeric<uint64_t> result = 0;
  for (const auto& output : outputs_) {
    result += output.amount;
  }
  return result;
}

std::optional<cardano_rpc::Tokens>
CardanoTransaction::GetTotalInputTokensAmount() const {
  cardano_rpc::Tokens result;
  for (auto& input : inputs_) {
    for (auto& token : input.utxo_tokens) {
      uint64_t new_token_sum = 0;
      if (!base::CheckAdd(result[token.first], token.second)
               .AssignIfValid(&new_token_sum)) {
        return std::nullopt;
      }
      result[token.first] = new_token_sum;
    }
  }
  return result;
}

std::optional<cardano_rpc::Tokens>
CardanoTransaction::GetTotalOutputTokensAmount() const {
  cardano_rpc::Tokens result;
  for (auto& output : outputs_) {
    for (auto& token : output.tokens) {
      uint64_t new_token_sum = 0;
      if (!base::CheckAdd(result[token.first], token.second)
               .AssignIfValid(&new_token_sum)) {
        return std::nullopt;
      }
      result[token.first] = new_token_sum;
    }
  }
  return result;
}

std::optional<CardanoAddress> CardanoTransaction::GetToAddress() const {
  if (auto* target_output = TargetOutput()) {
    return target_output->address;
  }

  return std::nullopt;
}

bool CardanoTransaction::IsSendTokenTransaction() const {
  if (auto* target_output = TargetOutput()) {
    return !target_output->tokens.empty();
  }

  return false;
}

void CardanoTransaction::AddInput(TxInput input) {
  inputs_.push_back(std::move(input));
}

void CardanoTransaction::AddInputs(std::vector<TxInput> inputs) {
  inputs_.reserve(inputs_.size() + inputs.size());
  for (auto& input : inputs) {
    inputs_.push_back(std::move(input));
  }
}

void CardanoTransaction::ClearInputs() {
  inputs_.clear();
}

base::flat_set<CardanoAddress> CardanoTransaction::GetInputAddresses() const {
  base::flat_set<CardanoAddress> input_addresses;
  for (auto& input : inputs()) {
    input_addresses.insert(input.utxo_address);
  }
  return input_addresses;
}

void CardanoTransaction::SetWitnesses(std::vector<TxWitness> witnesses) {
  witnesses_ = std::move(witnesses);
}

void CardanoTransaction::AddWitness(TxWitness witness) {
  witnesses_.push_back(std::move(witness));
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

bool CardanoTransaction::EnsureTokensInChangeOutput() {
  auto input_tokens = GetTotalInputTokensAmount();
  if (!input_tokens) {
    return false;
  }

  // We already assigned some input tokens to target output, should not move
  // them to change.
  CHECK(TargetOutput());
  if (!SubtractTokens(*input_tokens, TargetOutput()->tokens)) {
    return false;
  }

  // OK if there is no tokens which must be assigned to change.
  if (input_tokens->empty()) {
    DCHECK(GetTotalInputTokensAmount() == GetTotalOutputTokensAmount());
    return true;
  }

  // Fail here as we must send tokens to change output, but there is no such
  // output.
  if (!ChangeOutput()) {
    return false;
  }

  ChangeOutput()->tokens = std::move(*input_tokens);

  DCHECK(GetTotalInputTokensAmount() == GetTotalOutputTokensAmount());

  return true;
}

std::optional<CardanoTxDecoder::SerializableTx>
CardanoTransaction::ToSerializableTx() const {
  CardanoTxDecoder::SerializableTx result;
  for (auto& input : inputs_) {
    result.tx_body.inputs.emplace_back();
    result.tx_body.inputs.back().tx_hash = input.utxo_outpoint.txid;
    result.tx_body.inputs.back().index = input.utxo_outpoint.index;
  }
  for (auto& output : outputs_) {
    result.tx_body.outputs.emplace_back(output.ToSerializableTxOutput());
  }

  result.tx_body.fee = fee_;
  result.tx_body.ttl = invalid_after_;

  for (auto& witness : witnesses_) {
    result.tx_witness.vkey_witness_set.emplace_back();
    result.tx_witness.vkey_witness_set.back().public_key = witness.public_key;
    result.tx_witness.vkey_witness_set.back().signature_bytes =
        witness.signature;
  }

  return result;
}

}  // namespace brave_wallet
