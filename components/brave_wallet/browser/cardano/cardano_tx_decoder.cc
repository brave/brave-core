/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_decoder.h"

#include <optional>
#include <vector>

#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "base/containers/to_vector.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.rs.h"

namespace brave_wallet {

namespace {

// https://cips.cardano.org/cip/CIP-0009#updatable-protocol-parameters
constexpr size_t kMaxCardanoTransactionSize = 16 * 1024;  // 16384

rust::Vec<uint8_t> ToRust(base::span<const uint8_t> data) {
  rust::Vec<uint8_t> vec;
  vec.reserve(data.size());
  std::ranges::copy(data, std::back_inserter(vec));
  return vec;
}

std::vector<uint8_t> FromRust(const rust::Vec<uint8_t>& vec) {
  return base::ToVector(vec);
}

CxxSerializableTxInput ToRust(
    const CardanoTxDecoder::SerializableTxInput& input) {
  CxxSerializableTxInput result;

  result.tx_hash = input.tx_hash;
  result.index = input.index;

  return result;
}

CardanoTxDecoder::SerializableTxInput FromRust(
    const CxxSerializableTxInput& input) {
  CardanoTxDecoder::SerializableTxInput result;

  result.tx_hash = input.tx_hash;
  result.index = input.index;

  return result;
}

CxxSerializableCoinValue ToRust(const cardano_rpc::CoinValue& coin_value) {
  CxxSerializableCoinValue result;

  result.lovelace_amount = coin_value.lovelace_amount;
  result.tokens.reserve(coin_value.tokens.size());
  for (const auto& token : coin_value.tokens) {
    CxxSerializableTxOutputToken token_rust;
    token_rust.token_id = ToRust(token.first);
    token_rust.amount = token.second;
    result.tokens.emplace_back(std::move(token_rust));
  }

  return result;
}

cardano_rpc::CoinValue FromRust(const CxxSerializableCoinValue& coin_value) {
  cardano_rpc::CoinValue result;

  result.lovelace_amount = coin_value.lovelace_amount;
  result.tokens.reserve(coin_value.tokens.size());
  for (const auto& token : coin_value.tokens) {
    result.tokens.insert(
        std::make_pair(FromRust(token.token_id), token.amount));
  }
  return result;
}

CxxSerializableTxOutput ToRust(
    const CardanoTxDecoder::SerializableTxOutput& output) {
  CxxSerializableTxOutput result;

  result.addr = ToRust(output.address_bytes);
  result.coin_value = ToRust(output.coin_value);

  return result;
}

CardanoTxDecoder::SerializableTxOutput FromRust(
    const CxxSerializableTxOutput& output) {
  CardanoTxDecoder::SerializableTxOutput result;

  result.address_bytes = FromRust(output.addr);
  result.coin_value = FromRust(output.coin_value);

  return result;
}

CxxSerializableWithdrawal ToRust(
    const CardanoTxDecoder::SerializableWithdrawal& withdrawal) {
  CxxSerializableWithdrawal result;
  result.reward_account = ToRust(withdrawal.reward_account);
  result.coin = withdrawal.coin;
  return result;
}

CardanoTxDecoder::SerializableWithdrawal FromRust(
    const CxxSerializableWithdrawal& withdrawal) {
  CardanoTxDecoder::SerializableWithdrawal result;
  result.reward_account = FromRust(withdrawal.reward_account);
  result.coin = withdrawal.coin;
  return result;
}

CxxSerializableMintToken ToRust(
    const CardanoTxDecoder::SerializableMintToken& mint_token) {
  CxxSerializableMintToken result;
  result.token_id = ToRust(mint_token.token_id);
  result.amount = mint_token.amount;
  return result;
}

CardanoTxDecoder::SerializableMintToken FromRust(
    const CxxSerializableMintToken& mint_token) {
  CardanoTxDecoder::SerializableMintToken result;
  result.token_id = FromRust(mint_token.token_id);
  result.amount = mint_token.amount;
  return result;
}

CxxSerializableTxBody ToRust(const CardanoTxDecoder::SerializableTxBody& body) {
  CxxSerializableTxBody result;

  result.inputs.reserve(body.inputs.size());
  for (const auto& input : body.inputs) {
    result.inputs.push_back(ToRust(input));
  }

  result.outputs.reserve(body.outputs.size());
  for (const auto& output : body.outputs) {
    result.outputs.push_back(ToRust(output));
  }

  result.fee = body.fee;
  result.ttl = body.ttl.value_or(0u);
  result.has_ttl = body.ttl.has_value();

  result.withdrawals.reserve(body.withdrawals.size());
  for (const auto& withdrawal : body.withdrawals) {
    result.withdrawals.push_back(ToRust(withdrawal));
  }

  result.mint.reserve(body.mint.size());
  for (const auto& mint_token : body.mint) {
    result.mint.push_back(ToRust(mint_token));
  }

  return result;
}

CardanoTxDecoder::SerializableTxBody FromRust(
    const CxxSerializableTxBody& body) {
  CardanoTxDecoder::SerializableTxBody result;

  result.inputs.reserve(body.inputs.size());
  for (const auto& input : body.inputs) {
    result.inputs.push_back(FromRust(input));
  }

  result.outputs.reserve(body.outputs.size());
  for (const auto& output : body.outputs) {
    result.outputs.push_back(FromRust(output));
  }

  result.fee = body.fee;
  result.ttl = body.has_ttl
                   ? std::make_optional(base::StrictNumeric<uint64_t>(body.ttl))
                   : std::nullopt;

  result.withdrawals.reserve(body.withdrawals.size());
  for (const auto& withdrawal : body.withdrawals) {
    result.withdrawals.push_back(FromRust(withdrawal));
  }

  result.mint.reserve(body.mint.size());
  for (const auto& mint_token : body.mint) {
    result.mint.push_back(FromRust(mint_token));
  }

  return result;
}

CxxSerializableVkeyWitness ToRust(
    const CardanoTxDecoder::SerializableVkeyWitness& from) {
  CxxSerializableVkeyWitness result;
  result.pubkey = from.public_key;
  result.signature = from.signature_bytes;
  return result;
}

CardanoTxDecoder::SerializableVkeyWitness FromRust(
    const CxxSerializableVkeyWitness& from) {
  CardanoTxDecoder::SerializableVkeyWitness result;
  result.public_key = from.pubkey;
  result.signature_bytes = from.signature;
  return result;
}

CxxSerializableTxWitness ToRust(
    const CardanoTxDecoder::SerializableTxWitness& from) {
  CxxSerializableTxWitness result;

  result.vkey_witness_set.reserve(from.vkey_witness_set.size());
  for (auto& item : from.vkey_witness_set) {
    result.vkey_witness_set.push_back(ToRust(item));
  }

  return result;
}

CardanoTxDecoder::SerializableTxWitness FromRust(
    const CxxSerializableTxWitness& from) {
  CardanoTxDecoder::SerializableTxWitness result;

  result.vkey_witness_set.reserve(from.vkey_witness_set.size());
  for (const auto& item : from.vkey_witness_set) {
    result.vkey_witness_set.push_back(FromRust(item));
  }

  return result;
}

CxxSerializableTx ToRust(const CardanoTxDecoder::SerializableTx& tx) {
  CxxSerializableTx result;
  result.body = ToRust(tx.tx_body);
  result.witness = ToRust(tx.tx_witness);
  return result;
}

CardanoTxDecoder::SerializableTx FromRust(const CxxSerializableTx& tx) {
  CardanoTxDecoder::SerializableTx result;
  result.tx_body = FromRust(tx.body);
  result.tx_witness = FromRust(tx.witness);

  return result;
}

bool HasDuplicateInputs(const CardanoTxDecoder::SerializableTx& tx) {
  auto inputs = tx.tx_body.inputs;
  std::sort(inputs.begin(), inputs.end());
  return std::adjacent_find(inputs.begin(), inputs.end()) != inputs.end();
}

}  // namespace

CardanoTxDecoder::SerializableTxInput::SerializableTxInput() = default;
CardanoTxDecoder::SerializableTxInput::SerializableTxInput(
    std::array<uint8_t, kCardanoTxHashSize> tx_hash,
    uint32_t index)
    : tx_hash(tx_hash), index(index) {}
CardanoTxDecoder::SerializableTxInput::~SerializableTxInput() = default;
CardanoTxDecoder::SerializableTxInput::SerializableTxInput(
    const CardanoTxDecoder::SerializableTxInput&) = default;
CardanoTxDecoder::SerializableTxInput&
CardanoTxDecoder::SerializableTxInput::operator=(
    const CardanoTxDecoder::SerializableTxInput&) = default;
CardanoTxDecoder::SerializableTxInput::SerializableTxInput(
    CardanoTxDecoder::SerializableTxInput&&) = default;
CardanoTxDecoder::SerializableTxInput&
CardanoTxDecoder::SerializableTxInput::operator=(
    CardanoTxDecoder::SerializableTxInput&&) = default;

CardanoTxDecoder::SerializableTxOutput::SerializableTxOutput() = default;
CardanoTxDecoder::SerializableTxOutput::SerializableTxOutput(
    std::vector<uint8_t> address_bytes,
    cardano_rpc::CoinValue coin_value)
    : address_bytes(std::move(address_bytes)),
      coin_value(std::move(coin_value)) {}
CardanoTxDecoder::SerializableTxOutput::~SerializableTxOutput() = default;
CardanoTxDecoder::SerializableTxOutput::SerializableTxOutput(
    const CardanoTxDecoder::SerializableTxOutput&) = default;
CardanoTxDecoder::SerializableTxOutput&
CardanoTxDecoder::SerializableTxOutput::operator=(
    const CardanoTxDecoder::SerializableTxOutput&) = default;
CardanoTxDecoder::SerializableTxOutput::SerializableTxOutput(
    CardanoTxDecoder::SerializableTxOutput&&) = default;
CardanoTxDecoder::SerializableTxOutput&
CardanoTxDecoder::SerializableTxOutput::operator=(
    CardanoTxDecoder::SerializableTxOutput&&) = default;

CardanoTxDecoder::SerializableWithdrawal::SerializableWithdrawal() = default;
CardanoTxDecoder::SerializableWithdrawal::SerializableWithdrawal(
    std::vector<uint8_t> reward_account,
    uint64_t coin)
    : reward_account(std::move(reward_account)), coin(coin) {}
CardanoTxDecoder::SerializableWithdrawal::~SerializableWithdrawal() = default;
CardanoTxDecoder::SerializableWithdrawal::SerializableWithdrawal(
    const SerializableWithdrawal&) = default;
CardanoTxDecoder::SerializableWithdrawal&
CardanoTxDecoder::SerializableWithdrawal::operator=(
    const SerializableWithdrawal&) = default;
CardanoTxDecoder::SerializableWithdrawal::SerializableWithdrawal(
    SerializableWithdrawal&&) = default;
CardanoTxDecoder::SerializableWithdrawal&
CardanoTxDecoder::SerializableWithdrawal::operator=(SerializableWithdrawal&&) =
    default;

CardanoTxDecoder::SerializableMintToken::SerializableMintToken() = default;
CardanoTxDecoder::SerializableMintToken::SerializableMintToken(
    std::vector<uint8_t> token_id,
    int64_t amount)
    : token_id(std::move(token_id)), amount(amount) {}
CardanoTxDecoder::SerializableMintToken::~SerializableMintToken() = default;
CardanoTxDecoder::SerializableMintToken::SerializableMintToken(
    const SerializableMintToken&) = default;
CardanoTxDecoder::SerializableMintToken&
CardanoTxDecoder::SerializableMintToken::operator=(
    const SerializableMintToken&) = default;
CardanoTxDecoder::SerializableMintToken::SerializableMintToken(
    SerializableMintToken&&) = default;
CardanoTxDecoder::SerializableMintToken&
CardanoTxDecoder::SerializableMintToken::operator=(SerializableMintToken&&) =
    default;

CardanoTxDecoder::SerializableTxBody::SerializableTxBody() = default;
CardanoTxDecoder::SerializableTxBody::~SerializableTxBody() = default;
CardanoTxDecoder::SerializableTxBody::SerializableTxBody(
    const SerializableTxBody&) = default;
CardanoTxDecoder::SerializableTxBody&
CardanoTxDecoder::SerializableTxBody::operator=(const SerializableTxBody&) =
    default;
CardanoTxDecoder::SerializableTxBody::SerializableTxBody(SerializableTxBody&&) =
    default;
CardanoTxDecoder::SerializableTxBody&
CardanoTxDecoder::SerializableTxBody::operator=(SerializableTxBody&&) = default;

CardanoTxDecoder::SerializableVkeyWitness::SerializableVkeyWitness() = default;
CardanoTxDecoder::SerializableVkeyWitness::~SerializableVkeyWitness() = default;
CardanoTxDecoder::SerializableVkeyWitness::SerializableVkeyWitness(
    base::span<const uint8_t, kCardanoSignatureSize> signature_bytes_arg,
    base::span<const uint8_t, kCardanoPubKeySize> public_key_arg) {
  base::span(signature_bytes).copy_from_nonoverlapping(signature_bytes_arg);
  base::span(public_key).copy_from_nonoverlapping(public_key_arg);
}
CardanoTxDecoder::SerializableVkeyWitness::SerializableVkeyWitness(
    const SerializableVkeyWitness&) = default;
CardanoTxDecoder::SerializableVkeyWitness&
CardanoTxDecoder::SerializableVkeyWitness::operator=(
    const SerializableVkeyWitness&) = default;
CardanoTxDecoder::SerializableVkeyWitness::SerializableVkeyWitness(
    SerializableVkeyWitness&&) = default;
CardanoTxDecoder::SerializableVkeyWitness&
CardanoTxDecoder::SerializableVkeyWitness::operator=(
    SerializableVkeyWitness&&) = default;

CardanoTxDecoder::SerializableTxWitness::SerializableTxWitness() = default;
CardanoTxDecoder::SerializableTxWitness::~SerializableTxWitness() = default;
CardanoTxDecoder::SerializableTxWitness::SerializableTxWitness(
    const SerializableTxWitness&) = default;
CardanoTxDecoder::SerializableTxWitness&
CardanoTxDecoder::SerializableTxWitness::operator=(
    const SerializableTxWitness&) = default;
CardanoTxDecoder::SerializableTxWitness::SerializableTxWitness(
    SerializableTxWitness&&) = default;
CardanoTxDecoder::SerializableTxWitness&
CardanoTxDecoder::SerializableTxWitness::operator=(SerializableTxWitness&&) =
    default;

CardanoTxDecoder::SerializableTx::SerializableTx() = default;
CardanoTxDecoder::SerializableTx::~SerializableTx() = default;
CardanoTxDecoder::SerializableTx::SerializableTx(const SerializableTx&) =
    default;
CardanoTxDecoder::SerializableTx& CardanoTxDecoder::SerializableTx::operator=(
    const SerializableTx&) = default;
CardanoTxDecoder::SerializableTx::SerializableTx(SerializableTx&&) = default;
CardanoTxDecoder::SerializableTx& CardanoTxDecoder::SerializableTx::operator=(
    SerializableTx&&) = default;

CardanoTxDecoder::DecodedTx::DecodedTx() = default;
CardanoTxDecoder::DecodedTx::~DecodedTx() = default;
CardanoTxDecoder::DecodedTx::DecodedTx(const DecodedTx&) = default;
CardanoTxDecoder::DecodedTx::DecodedTx(DecodedTx&&) = default;

CardanoTxDecoder::CardanoTxDecoder() = default;
CardanoTxDecoder::~CardanoTxDecoder() = default;

// static
void CardanoTxDecoder::SetUseSetTagForTesting(bool enable) {
  CHECK_IS_TEST();
  use_set_tag_for_testing(enable);  // IN-TEST
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::EncodeTransaction(
    const SerializableTx& tx) {
  // Inputs come as vector but serialized as a set of supposed to be unique
  // elements. We must ensure there is no duplicates and fail fast.
  if (HasDuplicateInputs(tx)) {
    return std::nullopt;
  }

  auto result = encode_cardano_transaction(ToRust(tx));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  return base::ToVector(result->bytes());
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::EncodeTransactionOutput(
    const SerializableTxOutput& output) {
  auto result = encode_cardano_transaction_output(ToRust(output));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  return base::ToVector(result->bytes());
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::EncodeUtxo(
    const SerializableTxInput& input,
    const SerializableTxOutput& output) {
  auto result = encode_cardano_utxo(ToRust(input), ToRust(output));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  return base::ToVector(result->bytes());
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::EncodeCoinValue(
    const cardano_rpc::CoinValue& coin_value) {
  auto result = encode_cardano_coin_value(ToRust(coin_value));
  if (!result->is_ok()) {
    return std::nullopt;
  }
  return base::ToVector(result->bytes());
}

// static
std::optional<cardano_rpc::CoinValue> CardanoTxDecoder::DecodeCoinValue(
    const std::vector<uint8_t>& coin_value_bytes) {
  auto result =
      decode_cardano_coin_value(base::SpanToRustSlice(coin_value_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }
  return FromRust(result->value());
}

// static
std::optional<std::array<uint8_t, kCardanoTxHashSize>>
CardanoTxDecoder::GetTransactionHash(const SerializableTx& tx) {
  return get_cardano_transaction_hash(ToRust(tx));
}

// static
std::optional<CardanoTxDecoder::DecodedTx> CardanoTxDecoder::DecodeTransaction(
    base::span<const uint8_t> cbor_bytes) {
  if (cbor_bytes.size() > kMaxCardanoTransactionSize) {
    return std::nullopt;
  }

  auto result = decode_cardano_transaction(base::SpanToRustSlice(cbor_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  DecodedTx decoded_tx;
  decoded_tx.tx = FromRust(result->tx());
  decoded_tx.raw_body_bytes = FromRust(result->raw_body());
  decoded_tx.raw_tx_bytes = FromRust(result->raw_tx());

  return decoded_tx;
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::EncodeWitness(
    const SerializableTxWitness& witness) {
  auto result = encode_cardano_witness(ToRust(witness));

  if (!result->is_ok()) {
    return std::nullopt;
  }

  return base::ToVector(result->bytes());
}

}  // namespace brave_wallet
