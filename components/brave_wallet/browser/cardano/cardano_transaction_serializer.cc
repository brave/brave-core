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

#include "base/check.h"
#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

// https://github.com/Emurgo/cardano-serialization-lib/blob/c8bb8f43a916d804b89c3e226560265b65f1689a/rust/src/utils.rs#L791
constexpr uint64_t kMinAdaUtxoConstantOverhead = 160;

namespace {

constexpr std::array<uint8_t, kCardanoWitnessSize> kDummyTxWitnessBytes = {};

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

cbor::Value::ArrayValue CardanoTransactionSerializer::SerializeOutput(
    const CardanoTransaction::TxOutput& output) {
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

  return output_value;
}

cbor::Value::ArrayValue CardanoTransactionSerializer::SerializeOutputs(
    const CardanoTransaction& tx) {
  cbor::Value::ArrayValue result;
  for (const auto& output : tx.outputs()) {
    result.emplace_back(SerializeOutput(output));
  }

  return result;
}

cbor::Value CardanoTransactionSerializer::SerializeTxBody(
    const CardanoTransaction& tx) {
  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/core/src/Serialization/TransactionBody/TransactionBody.ts#L75-L250

  cbor::Value::MapValue body_map;

  body_map.emplace(0, SerializeInputs(tx));
  body_map.emplace(1, SerializeOutputs(tx));
  body_map.emplace(2,
                   options_.max_value_for_fee
                       ? std::numeric_limits<int64_t>::max()
                       : AmountToInt64ForCbor(tx.EffectiveFeeAmount()));  // fee
  body_map.emplace(3, base::strict_cast<int64_t>(tx.invalid_after()));    // ttl

  return cbor::Value(body_map);
}

cbor::Value CardanoTransactionSerializer::SerializeWitnessSet(
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

std::vector<uint8_t> CardanoTransactionSerializer::SerializeTransaction(
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

uint32_t CardanoTransactionSerializer::CalcTransactionSize(
    const CardanoTransaction& tx) {
  return SerializeTransaction(tx).size();
}

std::array<uint8_t, kCardanoTxHashSize> CardanoTransactionSerializer::GetTxHash(
    const CardanoTransaction& tx) {
  auto cbor_bytes = cbor::Writer::Write(SerializeTxBody(tx));
  CHECK(cbor_bytes);
  return Blake2bHash<kCardanoTxHashSize>({*cbor_bytes});
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

std::optional<uint64_t> CardanoTransactionSerializer::CalcMinAdaRequired(
    const CardanoTransaction::TxOutput& output,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  // https://github.com/Emurgo/cardano-serialization-lib/blob/c8bb8f43a916d804b89c3e226560265b65f1689a/rust/src/utils.rs#L767

  CardanoTransaction::TxOutput cur_output = output;
  // We need 5 iterations as uint64 can be encoded in CBOR in 1, 2, 3, 5, or 9
  // bytes.
  for (int i = 0; i < 4; i++) {
    auto cbor_bytes =
        cbor::Writer::Write(cbor::Value(SerializeOutput(cur_output)));
    CHECK(cbor_bytes);

    uint64_t required_lovelace = 0;
    if (!base::CheckMul(epoch_parameters.coins_per_utxo_size,
                        base::CheckAdd<uint64_t>(cbor_bytes->size(),
                                                 kMinAdaUtxoConstantOverhead))
             .AssignIfValid(&required_lovelace)) {
      return std::nullopt;
    }

    if (cur_output.amount < required_lovelace) {
      // Current output amount is less than required lovelace. But larger
      // required lovelace may produce larger cbor binary for this output. So we
      // increase the amount and run loop again.
      cur_output.amount = required_lovelace;
    } else {
      return required_lovelace;
    }
  }

  return std::nullopt;
}

bool CardanoTransactionSerializer::ValidateMinValue(
    const CardanoTransaction::TxOutput& output,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  auto min_ada_required = CardanoTransactionSerializer().CalcMinAdaRequired(
      output, epoch_parameters);
  return min_ada_required && output.amount >= min_ada_required.value();
}

}  // namespace brave_wallet
