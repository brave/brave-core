/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.rs.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {
std::optional<CardanoTxDecoder::RestoredTransactionInput> ConvertInput(
    const CxxRestoredCardanoInput& cxx_input) {
  if (cxx_input.tx_hash.size() != 32u) {
    return std::nullopt;
  }

  CardanoTxDecoder::RestoredTransactionInput restored_input;
  std::copy(cxx_input.tx_hash.begin(), cxx_input.tx_hash.end(),
            restored_input.tx_hash.begin());
  restored_input.index = cxx_input.index;

  return restored_input;
}

CardanoTxDecoder::RestoredTransactionOutput ConvertOutput(
    const CxxRestoredCardanoOutput& cxx_output) {
  CardanoTxDecoder::RestoredTransactionOutput restored_output;

  // Extract addr and amount
  // Convert addr from vector<uint8_t> to CardanoAddress using FromCborBytes
  if (!cxx_output.addr.empty()) {
    auto address = CardanoAddress::FromCborBytes(cxx_output.addr);
    if (address) {
      restored_output.address = std::move(address.value());
    }
  }

  restored_output.amount = cxx_output.amount;

  // Return RestoredTransactionOutput
  return restored_output;
}

std::optional<CardanoTxDecoder::RestoredTransactionBody> ConvertBody(
    const ::brave_wallet::CxxRestoredCardanoBody& cxx_body) {
  CardanoTxDecoder::RestoredTransactionBody restored_body;

  // Convert all inputs using ConvertInput
  restored_body.inputs.reserve(cxx_body.inputs.size());
  for (const auto& input : cxx_body.inputs) {
    auto converted_input = ConvertInput(input);
    if (!converted_input) {
      return std::nullopt;
    }
    restored_body.inputs.push_back(std::move(*converted_input));
  }

  // Convert all outputs using ConvertOutput
  restored_body.outputs.reserve(cxx_body.outputs.size());
  for (const auto& output : cxx_body.outputs) {
    restored_body.outputs.push_back(ConvertOutput(output));
  }

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
  auto result = decode_cardano_transaction(
      ::rust::Slice<const uint8_t>{cbor_bytes.data(), cbor_bytes.size()});

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
  restored_tx.raw_bytes = base::ToVector(cbor_bytes);

  return restored_tx;
}

// CardanoSignMessageResult implementations
CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult() =
    default;

CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult(
    const CardanoSignMessageResult&) = default;

CardanoTxDecoder::CardanoSignMessageResult::CardanoSignMessageResult(
    CardanoSignMessageResult&&) = default;

CardanoTxDecoder::CardanoSignMessageResult::~CardanoSignMessageResult() =
    default;

// static
std::optional<std::vector<uint8_t>> CardanoTxDecoder::ApplySignResults(
    const std::vector<uint8_t>& unsigned_tx_bytes,
    const std::vector<CardanoSignMessageResult>& witness_results) {
  // Convert CardanoSignMessageResult to CxxWittness
  std::vector<CxxWittness> cxx_witnesses;
  cxx_witnesses.reserve(witness_results.size());

  for (const auto& witness_result : witness_results) {
    // Skip failed signing results
    if (witness_result.signature_bytes.empty() ||
        witness_result.public_key.empty()) {
      continue;
    }

    CxxWittness cxx_witness;
    // Convert std::vector<uint8_t> to Rust Vec<unsigned char> using
    // ranges::copy
    cxx_witness.pubkey.reserve(witness_result.public_key.size());
    std::ranges::copy(witness_result.public_key,
                      std::back_inserter(cxx_witness.pubkey));

    cxx_witness.signature.reserve(witness_result.signature_bytes.size());
    std::ranges::copy(witness_result.signature_bytes,
                      std::back_inserter(cxx_witness.signature));

    cxx_witnesses.push_back(std::move(cxx_witness));
  }

  // Convert std::vector to Rust Vec for the function call using ranges::copy
  ::rust::Vec<CxxWittness> rust_witnesses;
  rust_witnesses.reserve(cxx_witnesses.size());
  std::ranges::copy(cxx_witnesses, std::back_inserter(rust_witnesses));

  // Call apply_signatures(unsigned_tx_bytes, witnesses) from Rust
  auto result =
      apply_signatures(::rust::Slice<const uint8_t>{unsigned_tx_bytes.data(),
                                                    unsigned_tx_bytes.size()},
                       std::move(rust_witnesses));

  // Check result->is_ok() to verify success
  if (!result->is_ok()) {
    return std::nullopt;
  }

  // If successful, call result->unwrap() to get CxxSignedCardanoTransaction
  auto signed_tx = result->unwrap();

  // Convert Rust Vec back to C++ vector
  auto rust_bytes = signed_tx->bytes();
  std::vector<uint8_t> cpp_bytes;
  cpp_bytes.reserve(rust_bytes.size());
  std::ranges::copy(rust_bytes, std::back_inserter(cpp_bytes));

  // Return signed transaction bytes
  return cpp_bytes;
}

}  // namespace brave_wallet
