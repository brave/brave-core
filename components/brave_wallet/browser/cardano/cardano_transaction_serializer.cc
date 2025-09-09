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
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "components/cbor/reader.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

namespace {

constexpr std::array<uint8_t, kCardanoWitnessSize> kDummyTxWitnessBytes = {};

}  // namespace

CardanoTransactionSerializer::RestoredTransactionInput::
    RestoredTransactionInput() = default;
CardanoTransactionSerializer::RestoredTransactionInput::
    ~RestoredTransactionInput() = default;
CardanoTransactionSerializer::RestoredTransactionInput::
    RestoredTransactionInput(
        CardanoTransactionSerializer::RestoredTransactionInput&) = default;
CardanoTransactionSerializer::RestoredTransactionInput::
    RestoredTransactionInput(
        CardanoTransactionSerializer::RestoredTransactionInput&&) = default;

CardanoTransactionSerializer::RestoredTransactionOutput::
    RestoredTransactionOutput() = default;
CardanoTransactionSerializer::RestoredTransactionOutput::
    ~RestoredTransactionOutput() = default;

CardanoTransactionSerializer::RestoredTransactionBody::
    RestoredTransactionBody() = default;
CardanoTransactionSerializer::RestoredTransactionBody::
    ~RestoredTransactionBody() = default;
CardanoTransactionSerializer::RestoredTransactionBody::RestoredTransactionBody(
    RestoredTransactionBody&) = default;
CardanoTransactionSerializer::RestoredTransactionBody::RestoredTransactionBody(
    RestoredTransactionBody&&) = default;
CardanoTransactionSerializer::RestoredTransactionBody&
CardanoTransactionSerializer::RestoredTransactionBody::operator=(
    RestoredTransactionBody&&) = default;

CardanoTransactionSerializer::RestoredTransaction::RestoredTransaction() =
    default;
CardanoTransactionSerializer::RestoredTransaction::~RestoredTransaction() =
    default;
CardanoTransactionSerializer::RestoredTransaction::RestoredTransaction(
    RestoredTransaction&) = default;
CardanoTransactionSerializer::RestoredTransaction::RestoredTransaction(
    RestoredTransaction&&) = default;

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
    // TODO(https://github.com/brave/brave-browser/issues/45278): upstream a fix
    // for cbor::Value to support uint64_t
    input_value.emplace_back(static_cast<int64_t>(input.utxo_outpoint.index));

    result.emplace_back(std::move(input_value));
  }

  return result;
}

std::optional<
    std::vector<CardanoTransactionSerializer::RestoredTransactionInput>>
CardanoTransactionSerializer::DeserializeInputs(
    const cbor::Value::ArrayValue& data) {
  std::vector<RestoredTransactionInput> tx_inputs;
  for (const auto& value : data) {
    if (!value.is_array() || value.GetArray().size() != 2) {
      return std::nullopt;
    }

    if (!value.GetArray()[0].is_bytestring()) {
      return std::nullopt;
    }

    if (value.GetArray()[0].GetBytestring().size() != kCardanoTxHashSize) {
      return std::nullopt;
    }

    if (!value.GetArray()[1].is_integer()) {
      return std::nullopt;
    }

    RestoredTransactionInput tx_input;
    base::span(tx_input.tx_hash)
        .copy_from_nonoverlapping(value.GetArray()[0].GetBytestring());
    tx_input.index = static_cast<uint64_t>(value.GetArray()[1].GetInteger());

    tx_inputs.push_back(std::move(tx_input));
  }
  return tx_inputs;
}

std::optional<
    std::vector<CardanoTransactionSerializer::RestoredTransactionOutput>>
CardanoTransactionSerializer::DeserializeOutputs(
    const cbor::Value::ArrayValue& data) {
  std::vector<RestoredTransactionOutput> outputs;
  for (const auto& value : data) {
    if (!value.is_array()) {
      return std::nullopt;
    }

    if (value.GetArray().size() != 2) {
      return std::nullopt;
    }

    if (!value.GetArray()[0].is_bytestring() ||
        !value.GetArray()[1].is_integer()) {
      return std::nullopt;
    }

    RestoredTransactionOutput output;
    auto addr =
        CardanoAddress::FromCborBytes(value.GetArray()[0].GetBytestring());
    if (!addr) {
      return std::nullopt;
    }
    output.address = std::move(addr.value());
    base::CheckedNumeric<uint64_t> amount = value.GetArray()[1].GetInteger();
    if (!amount.IsValid()) {
      return std::nullopt;
    }
    output.amount = amount.ValueOrDie();
    outputs.push_back(std::move(output));
  }

  return outputs;
}

std::optional<std::vector<CardanoTransactionSerializer::InputWitness>>
CardanoTransactionSerializer::DeserializeWitnessSet(
    const cbor::Value::ArrayValue& data) {
  std::vector<CardanoTransactionSerializer::InputWitness> witness_set;
  for (const auto& value : data) {
    if (!value.is_array()) {
      return std::nullopt;
    }

    if (value.GetArray().size() != 2) {
      return std::nullopt;
    }

    if (!value.GetArray()[0].is_bytestring() ||
        !value.GetArray()[1].is_bytestring()) {
      return std::nullopt;
    }

    auto& pubkey = value.GetArray()[0].GetBytestring();
    auto& signature = value.GetArray()[1].GetBytestring();

    if (pubkey.size() != kEd25519PublicKeySize ||
        signature.size() != kEd25519SignatureSize) {
      return std::nullopt;
    }

    InputWitness witness;

    base::SpanWriter span_writer = base::SpanWriter(base::span(witness));
    span_writer.Write(pubkey);
    span_writer.Write(signature);
    CHECK_EQ(0u, span_writer.remaining());

    witness_set.push_back(std::move(witness));
  }

  return witness_set;
}

std::optional<CardanoTransactionSerializer::RestoredTransactionBody>
CardanoTransactionSerializer::DeserializeTxBody(
    const cbor::Value::MapValue& data) {
  if (data.size() != 4) {
    return std::nullopt;
  }

  cbor::Value::MapValue::const_iterator map_iterator =
      data.find(cbor::Value(0));
  if (map_iterator == data.end() || !map_iterator->second.is_array()) {
    return std::nullopt;
  }

  auto inputs = DeserializeInputs(map_iterator->second.GetArray());

  if (!inputs) {
    return std::nullopt;
  }

  map_iterator = data.find(cbor::Value(1));
  if (map_iterator == data.end() || !map_iterator->second.is_array()) {
    return std::nullopt;
  }
  auto outputs = DeserializeOutputs(map_iterator->second.GetArray());

  RestoredTransactionBody result;
  result.inputs_ = std::move(inputs.value());
  result.outputs_ = std::move(outputs.value());

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
      output_value.emplace_back(static_cast<int64_t>(output.amount));
    }

    result.emplace_back(std::move(output_value));
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
                       : static_cast<int64_t>(tx.EffectiveFeeAmount()));  // fee
  body_map.emplace(3, static_cast<int64_t>(tx.invalid_after()));          // ttl

  return cbor::Value(body_map);
}

cbor::Value CardanoTransactionSerializer::SerializeWitnessSet(
    const CardanoTransaction& tx) {
  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/core/src/Serialization/TransactionWitnessSet/TransactionWitnessSet.ts#L49-L116

  cbor::Value::MapValue witness_map;

  // Verification Key Witness array
  cbor::Value::ArrayValue vk_witness_array;

  if (options_.ignore_sign_check_for_testing) {
    CHECK_IS_TEST();

    for (const auto& witness : tx.witnesses()) {
      cbor::Value::ArrayValue input_array;
      auto [pubkey, signature] =
          base::span(witness.witness_bytes).split_at<kEd25519PublicKeySize>();
      input_array.emplace_back(pubkey);
      input_array.emplace_back(signature);
      vk_witness_array.emplace_back(std::move(input_array));
    }
  } else if (options_.use_dummy_witness_set) {
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

// Deserializes a Cardano transaction from a byte vector (CBOR format).
std::optional<CardanoTransactionSerializer::RestoredTransaction>
CardanoTransactionSerializer::DeserializeTransaction(
    const std::vector<uint8_t>& bytes) {
  std::optional<cbor::Value> as_cbor = cbor::Reader::Read(bytes);

  if (!as_cbor || !as_cbor->is_array()) {
    return std::nullopt;
  }

  const std::vector<cbor::Value>& array_value = as_cbor->GetArray();

  if (array_value.size() != 4) {
    return std::nullopt;
  }

  if (!array_value[0].is_map() || !array_value[1].is_map()) {
    return std::nullopt;
  }

  auto tx_body = DeserializeTxBody(array_value[0].GetMap());
  if (!tx_body) {
    return std::nullopt;
  }

  RestoredTransaction result;

  result.raw_bytes_ = bytes;
  result.tx_body_ = std::move(tx_body.value());

  return result;
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
  uint64_t tx_size = CalcTransactionSize(tx);
  return tx_size * epoch_parameters.min_fee_coefficient +
         epoch_parameters.min_fee_constant;
}

std::optional<std::vector<uint8_t>>
CardanoTransactionSerializer::ApplySignResults(
    const std::vector<uint8_t>& unsigned_tx_bytes,
    const std::vector<CardanoSignMessageResult>& sign_results) {
  std::optional<cbor::Value> as_cbor = cbor::Reader::Read(unsigned_tx_bytes);
  cbor::Value::ArrayValue output;

  if (!as_cbor->is_array()) {
    return std::nullopt;
  }

  const auto& as_cbor_array = as_cbor->GetArray();

  if (as_cbor_array.size() != 4) {
    return std::nullopt;
  }

  output.emplace_back(as_cbor_array[0].Clone());

  if (!as_cbor_array[1].is_map()) {
    return std::nullopt;
  }

  const auto& witness_map = as_cbor_array[1].GetMap();
  auto witness_array = witness_map.find(cbor::Value(0));
  if (witness_array == witness_map.end()) {
    return std::nullopt;
  }

  if (!witness_array->second.is_array()) {
    return std::nullopt;
  }

  cbor::Value::ArrayValue output_witness_array;
  for (const auto& existing_signatures : witness_array->second.GetArray()) {
    output_witness_array.push_back(existing_signatures.Clone());
  }

  for (const auto& sign_result : sign_results) {
    cbor::Value::ArrayValue array;
    array.push_back(cbor::Value(sign_result.pubkey));
    array.push_back(cbor::Value(sign_result.signature));

    output_witness_array.emplace_back(std::move(array));
  }

  cbor::Value::MapValue output_witness_map;
  output_witness_map.emplace(0, std::move(output_witness_array));

  output.emplace_back(std::move(output_witness_map));
  output.emplace_back(as_cbor_array[2].Clone());
  output.emplace_back(as_cbor_array[3].Clone());

  std::optional<std::vector<uint8_t>> output_bytes =
      cbor::Writer::Write(cbor::Value(std::move(output)));
  CHECK(output_bytes);
  return *output_bytes;
}

}  // namespace brave_wallet
