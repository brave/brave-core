/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"
#include "components/cbor/values.h"

namespace brave_wallet {

namespace {

// Chromium's cbor component does not support uint64_t values, only int64_t. We
// need to be able to store uint64_t amounts in cbor.
// Casting it to int64_t is safe as total amount of lovelace supply is 45*10^15
// and is less than max(UINT_64) ~ 1.84 * 10^19.
int64_t AmountToInt64ForCbor(uint64_t amount) {
  return base::checked_cast<int64_t>(amount);
}

}  // namespace

CardanoTransactionSerializer::CardanoTransactionSerializer() = default;
CardanoTransactionSerializer::CardanoTransactionSerializer(
    CardanoTransactionSerializer::Options options)
    : options_(options) {}

cbor::Value::ArrayValue CardanoTransactionSerializer::SerializeInputs(
    const CardanoTransaction& tx) {
  cbor::Value::ArrayValue result;
  for (const auto& input : tx.inputs()) {
    cbor::Value::ArrayValue input_value;
    input_value.emplace_back(input.utxo_outpoint.txid);
    input_value.emplace_back(
        base::strict_cast<int64_t>(input.utxo_outpoint.index));
    result.emplace_back(std::move(input_value));
  }

  return result;
}

cbor::Value::ArrayValue CardanoTransactionSerializer::SerializeOutputs(
    const CardanoTransaction& tx) {
  cbor::Value::ArrayValue result;
  for (const auto& output : tx.outputs()) {
    cbor::Value::ArrayValue output_value;
    output_value.emplace_back(output.address.ToCborBytes());

    bool serialize_as_max_value =
        (output.type == CardanoTransaction::TxOutputType::kTarget &&
         options_.max_value_for_target_output) ||
        (output.type == CardanoTransaction::TxOutputType::kChange &&
         options_.max_value_for_change_output);

    if (serialize_as_max_value) {
      output_value.emplace_back(std::numeric_limits<int64_t>::max());
    } else {
      output_value.emplace_back(AmountToInt64ForCbor(output.amount));
    }

    result.emplace_back(std::move(output_value));
  }

  return result;
}

std::optional<std::vector<uint8_t>>
CardanoTransactionSerializer::SerializeTransaction(
    const CardanoTransaction& tx) {
  auto serializable_tx = tx.ToSerializableTx();
  if (!serializable_tx) {
    return std::nullopt;
  }
  return CardanoTxDecoder::EncodeTransaction(*serializable_tx);
}

uint32_t CardanoTransactionSerializer::CalcTransactionSize(
    const CardanoTransaction& tx) {
  return SerializeTransaction(tx)->size();
}

std::optional<std::array<uint8_t, kCardanoTxHashSize>>
CardanoTransactionSerializer::GetTxHash(const CardanoTransaction& tx) {
  auto serializable_tx = tx.ToSerializableTx();
  if (!serializable_tx) {
    return std::nullopt;
  }

  return CardanoTxDecoder::GetTransactionHash(*serializable_tx);
}

uint64_t CardanoTransactionSerializer::CalcMinTransactionFee(
    const CardanoTransaction& tx,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  base::CheckedNumeric<uint64_t> tx_size = CalcTransactionSize(tx);
  base::CheckedNumeric<uint64_t> fee =
      tx_size * epoch_parameters.min_fee_coefficient +
      epoch_parameters.min_fee_constant;
  return fee.ValueOrDie();
}

}  // namespace brave_wallet
