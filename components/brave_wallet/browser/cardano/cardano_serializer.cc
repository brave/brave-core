/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

namespace {

constexpr std::array<uint8_t, kCardanoWitnessSize> kDummyTxWitnessBytes = {};

}  // namespace

CardanoSerializer::CardanoSerializer() = default;
CardanoSerializer::CardanoSerializer(CardanoSerializer::Options options)
    : options_(options) {}

cbor::Value::ArrayValue CardanoSerializer::SerializeInputs(
    const CardanoTransaction& tx) {
  cbor::Value::ArrayValue result;
  for (const auto& input : tx.inputs()) {
    cbor::Value::ArrayValue input_value;
    input_value.emplace_back(input.utxo_outpoint.txid);
    // TODO(https://github.com/brave/brave-browser/issues/45278): upstream a fix
    // for cbor::Value to support uint64_t
    input_value.emplace_back(static_cast<int64_t>(input.utxo_outpoint.index));
    result.emplace_back(std::move(input_value));
  }

  return result;
}

cbor::Value::ArrayValue CardanoSerializer::SerializeOutputs(
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
      output_value.emplace_back(static_cast<int64_t>(output.amount));
    }

    result.emplace_back(std::move(output_value));
  }

  return result;
}

cbor::Value CardanoSerializer::SerializeTxBody(const CardanoTransaction& tx) {
  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/core/src/Serialization/TransactionBody/TransactionBody.ts#L75-L250

  cbor::Value::MapValue body_map;

  body_map.emplace(0, SerializeInputs(tx));
  body_map.emplace(1, SerializeOutputs(tx));
  body_map.emplace(2,
                   options_.max_value_for_fee
                       ? std::numeric_limits<int64_t>::max()
                       : static_cast<int64_t>(tx.EffectiveFeeAmount()));  // fee
  body_map.emplace(3, static_cast<int64_t>(tx.invalid_after()));          // ttl

  return cbor::Value(body_map);
}

cbor::Value CardanoSerializer::SerializeWitnessSet(
    const CardanoTransaction& tx) {
  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/core/src/Serialization/TransactionWitnessSet/TransactionWitnessSet.ts#L49-L116

  cbor::Value::MapValue witness_map;

  // Verification Key Witness array
  cbor::Value::ArrayValue vk_witness_array;

  if (options_.use_dummy_witness_set) {
    // Serialize with dummy signatures for size calculation.
    for (const auto& _ : tx.inputs()) {
      cbor::Value::ArrayValue input_array;
      auto [pubkey, signature] =
          base::span(kDummyTxWitnessBytes).split_at<kEd25519PublicKeySize>();
      input_array.emplace_back(pubkey);
      input_array.emplace_back(signature);
      vk_witness_array.emplace_back(std::move(input_array));
    }
  } else {
    DCHECK(tx.IsSigned());

    for (const auto& witness : tx.witnesses()) {
      cbor::Value::ArrayValue input_array;
      auto [pubkey, signature] =
          base::span(witness.witness_bytes).split_at<kEd25519PublicKeySize>();
      input_array.emplace_back(pubkey);
      input_array.emplace_back(signature);
      vk_witness_array.emplace_back(std::move(input_array));
    }
  }

  witness_map.emplace(0, std::move(vk_witness_array));

  return cbor::Value(witness_map);
}

std::vector<uint8_t> CardanoSerializer::SerializeTransaction(
    const CardanoTransaction& tx) {
  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/core/src/Serialization/Transaction.ts#L59-L84

  cbor::Value::ArrayValue transaction_array;
  transaction_array.push_back(SerializeTxBody(tx));
  transaction_array.push_back(SerializeWitnessSet(tx));
  transaction_array.emplace_back(true);  // Valid flag.
  transaction_array.emplace_back(
      cbor::Value::SimpleValue::NULL_VALUE);  // Auxilary data.

  std::optional<std::vector<uint8_t>> cbor_bytes =
      cbor::Writer::Write(cbor::Value(std::move(transaction_array)));
  CHECK(cbor_bytes);
  return *cbor_bytes;
}

uint32_t CardanoSerializer::CalcTransactionSize(const CardanoTransaction& tx) {
  return SerializeTransaction(tx).size();
}

std::array<uint8_t, kCardanoTxHashSize> CardanoSerializer::GetTxHash(
    const CardanoTransaction& tx) {
  auto cbor_bytes = cbor::Writer::Write(SerializeTxBody(tx));
  CHECK(cbor_bytes);
  return Blake2bHash<kCardanoTxHashSize>({*cbor_bytes});
}

uint64_t CardanoSerializer::CalcMinTransactionFee(
    const CardanoTransaction& tx,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  uint64_t tx_size = CalcTransactionSize(tx);
  return tx_size * epoch_parameters.min_fee_coefficient +
         epoch_parameters.min_fee_constant;
}

}  // namespace brave_wallet
