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
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

namespace {
std::optional<CardanoTxDecoder::RestoredTransactionInput> ConvertInput(
    const CxxRestoredCardanoInput& cxx_input) {
  CardanoTxDecoder::RestoredTransactionInput restored_input;
  if (cxx_input.tx_hash.size() != restored_input.tx_hash.size()) {
    return std::nullopt;
  }
  std::ranges::copy(cxx_input.tx_hash, restored_input.tx_hash.begin());
  restored_input.index = cxx_input.index;
  return restored_input;
}

std::optional<CardanoTxDecoder::RestoredTransactionOutput> ConvertOutput(
    const CxxRestoredCardanoOutput& cxx_output) {
  CardanoTxDecoder::RestoredTransactionOutput restored_output;

  if (!cxx_output.addr.empty()) {
    auto address = CardanoAddress::FromCborBytes(cxx_output.addr);
    if (!address) {
      return std::nullopt;
    }
    restored_output.address = std::move(address.value());
  }

  restored_output.amount = cxx_output.amount;

  return restored_output;
}

std::optional<CardanoTxDecoder::RestoredTransactionBody> ConvertBody(
    const CxxRestoredCardanoBody& cxx_body) {
  CardanoTxDecoder::RestoredTransactionBody restored_body;

  restored_body.inputs.reserve(cxx_body.inputs.size());
  for (const auto& input : cxx_body.inputs) {
    auto converted_input = ConvertInput(input);
    if (!converted_input) {
      return std::nullopt;
    }
    restored_body.inputs.push_back(std::move(*converted_input));
  }

  restored_body.outputs.reserve(cxx_body.outputs.size());
  for (const auto& output : cxx_body.outputs) {
    auto converted_output = ConvertOutput(output);
    if (!converted_output) {
      return std::nullopt;
    }
    restored_body.outputs.push_back(std::move(*converted_output));
  }

  restored_body.raw_body_bytes = base::ToVector(cxx_body.raw_body);

  return restored_body;
}

}  // namespace

CardanoTxDecoder::RestoredTransactionInput::RestoredTransactionInput() =
    default;
CardanoTxDecoder::RestoredTransactionInput::~RestoredTransactionInput() =
    default;
CardanoTxDecoder::RestoredTransactionInput::RestoredTransactionInput(
    const CardanoTxDecoder::RestoredTransactionInput&) = default;
CardanoTxDecoder::RestoredTransactionInput::RestoredTransactionInput(
    CardanoTxDecoder::RestoredTransactionInput&&) = default;

CardanoTxDecoder::RestoredTransactionOutput::RestoredTransactionOutput() =
    default;
CardanoTxDecoder::RestoredTransactionOutput::~RestoredTransactionOutput() =
    default;

CardanoTxDecoder::RestoredTransactionBody::RestoredTransactionBody() = default;
CardanoTxDecoder::RestoredTransactionBody::~RestoredTransactionBody() = default;
CardanoTxDecoder::RestoredTransactionBody::RestoredTransactionBody(
    const RestoredTransactionBody&) = default;
CardanoTxDecoder::RestoredTransactionBody::RestoredTransactionBody(
    RestoredTransactionBody&&) = default;
CardanoTxDecoder::RestoredTransactionBody&
CardanoTxDecoder::RestoredTransactionBody::operator=(
    RestoredTransactionBody&&) = default;

CardanoTxDecoder::RestoredTransaction::RestoredTransaction() = default;
CardanoTxDecoder::RestoredTransaction::~RestoredTransaction() = default;
CardanoTxDecoder::RestoredTransaction::RestoredTransaction(
    const RestoredTransaction&) = default;
CardanoTxDecoder::RestoredTransaction::RestoredTransaction(
    RestoredTransaction&&) = default;

CardanoTxDecoder::CardanoTxDecoder() = default;
CardanoTxDecoder::~CardanoTxDecoder() = default;

// static
std::optional<CardanoTxDecoder::RestoredTransaction>
CardanoTxDecoder::DecodeTransaction(base::span<const uint8_t> cbor_bytes) {
  auto result = decode_cardano_transaction(base::SpanToRustSlice(cbor_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto decoded_tx = result->unwrap();

  auto converted_body = ConvertBody(decoded_tx->tx_body());
  if (!converted_body) {
    return std::nullopt;
  }

  RestoredTransaction restored_tx;
  restored_tx.tx_body = std::move(*converted_body);
  restored_tx.raw_tx_bytes = base::ToVector(cbor_bytes);

  return restored_tx;
}

// CardanoSignMessageResult implementations
CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult() =
    default;

CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult(
    std::vector<uint8_t> signature_bytes,
    std::vector<uint8_t> public_key)
    : signature_bytes(std::move(signature_bytes)),
      public_key(std::move(public_key)) {}

CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult(
    const CardanoSignMessageResult&) = default;

CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult(
    CardanoSignMessageResult&&) = default;

CardanoTxDecoder::CardanoSignMessageResult::~CardanoSignMessageResult() =
    default;

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::AddWitnessesToTransaction(
    const std::vector<uint8_t>& unsigned_tx_bytes,
    const std::vector<CardanoSignMessageResult>& witness_results) {
  std::vector<CxxWitness> cxx_witnesses;
  cxx_witnesses.reserve(witness_results.size());

  for (const auto& witness_result : witness_results) {
    if (witness_result.signature_bytes.empty() ||
        witness_result.public_key.empty()) {
      continue;
    }

    CxxWitness cxx_witness;
    cxx_witness.pubkey.reserve(witness_result.public_key.size());
    std::ranges::copy(witness_result.public_key,
                      std::back_inserter(cxx_witness.pubkey));

    cxx_witness.signature.reserve(witness_result.signature_bytes.size());
    std::ranges::copy(witness_result.signature_bytes,
                      std::back_inserter(cxx_witness.signature));

    cxx_witnesses.push_back(std::move(cxx_witness));
  }

  ::rust::Vec<CxxWitness> rust_witnesses;
  rust_witnesses.reserve(cxx_witnesses.size());
  std::ranges::copy(cxx_witnesses, std::back_inserter(rust_witnesses));

  auto result = apply_signatures(base::SpanToRustSlice(unsigned_tx_bytes),
                                 std::move(rust_witnesses));

  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto signed_tx = result->unwrap();
  return base::ToVector(signed_tx->bytes());
}

}  // namespace brave_wallet
