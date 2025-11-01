/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"

#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "base/containers/to_vector.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.rs.h"

namespace brave_wallet {

namespace {

rust::Vec<uint8_t> ToRust(base::span<const uint8_t> data) {
  rust::Vec<uint8_t> vec;
  vec.reserve(data.size());
  std::ranges::copy(data, std::back_inserter(vec));
  return vec;
}

std::vector<uint8_t> FromRust(const rust::Vec<uint8_t>& vec) {
  return base::ToVector(vec);
}

CardanoTxDecoder::SerializableTxInput FromRust(
    const cardano_rust::SerializableTxInput& input) {
  CardanoTxDecoder::SerializableTxInput result;

  result.tx_hash = input.tx_hash;
  result.index = input.index;

  return result;
}

CardanoTxDecoder::SerializableTxOutput FromRust(
    const cardano_rust::SerializableTxOutput& output) {
  CardanoTxDecoder::SerializableTxOutput result;

  result.address_bytes = FromRust(output.addr);
  result.amount = output.amount;

  return result;
}

CardanoTxDecoder::SerializableTxBody FromRust(
    const cardano_rust::SerializableTxBody& body) {
  CardanoTxDecoder::SerializableTxBody result;

  result.inputs.reserve(body.inputs.size());
  for (const auto& input : body.inputs) {
    result.inputs.push_back(FromRust(input));
  }

  result.outputs.reserve(body.outputs.size());
  for (const auto& output : body.outputs) {
    result.outputs.push_back(FromRust(output));
  }

  return result;
}

CardanoTxDecoder::SerializableTx FromRust(
    const cardano_rust::SerializableTx& tx) {
  CardanoTxDecoder::SerializableTx result;
  result.tx_body = FromRust(tx.body);

  return result;
}

cardano_rust::SerializableVkeyWitness ToRust(
    const CardanoTxDecoder::SerializableVkeyWitness& from) {
  cardano_rust::SerializableVkeyWitness result;
  result.pubkey = ToRust(from.public_key);
  result.signature = ToRust(from.signature_bytes);
  return result;
}

cardano_rust::SerializableTxWitness ToRust(
    const CardanoTxDecoder::SerializableTxWitness& from) {
  cardano_rust::SerializableTxWitness result;

  result.vkey_witness_set.reserve(from.vkey_witness_set.size());
  for (auto& item : from.vkey_witness_set) {
    result.vkey_witness_set.push_back(ToRust(item));
  }

  return result;
}

}  // namespace

CardanoTxDecoder::SerializableTxInput::SerializableTxInput() = default;
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
std::optional<CardanoTxDecoder::DecodedTx> CardanoTxDecoder::DecodeTransaction(
    base::span<const uint8_t> cbor_bytes) {
  auto result = cardano_rust::decode_cardano_transaction(
      base::SpanToRustSlice(cbor_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto decoded_tx_rust = result->unwrap();

  DecodedTx decoded_tx;
  decoded_tx.tx = FromRust(decoded_tx_rust->tx());
  decoded_tx.raw_body_bytes = FromRust(decoded_tx_rust->raw_body());
  decoded_tx.raw_tx_bytes = FromRust(decoded_tx_rust->raw_tx());

  return decoded_tx;
}

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::AddWitnessesToTransaction(
    const std::vector<uint8_t>& tx_cbor_bytes,
    const SerializableTxWitness& witness) {
  auto result =
      apply_signatures(base::SpanToRustSlice(tx_cbor_bytes), ToRust(witness));

  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto signed_tx = result->unwrap();
  return base::ToVector(signed_tx->bytes());
}

}  // namespace brave_wallet
