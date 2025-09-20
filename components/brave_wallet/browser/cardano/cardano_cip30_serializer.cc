/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

#include <cstdint>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/cbor/reader.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

namespace {
std::vector<uint8_t> MakeSerializedProtectedHeaders(
    const CardanoAddress& payment_address) {
  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
  constexpr int kAlgHeaderKey = 1;
  constexpr int kAlgHeaderValueEdDSA = -8;
  constexpr int kKidHeaderKey = 4;
  constexpr char kAddressHeaderKey[] = "address";

  cbor::Value::MapValue protected_headers;
  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
  protected_headers.emplace(kAlgHeaderKey, kAlgHeaderValueEdDSA);
  protected_headers.emplace(kKidHeaderKey, payment_address.ToCborBytes());
  protected_headers.emplace(kAddressHeaderKey, payment_address.ToCborBytes());

  auto protected_headers_serialized =
      cbor::Writer::Write(cbor::Value(std::move(protected_headers)));
  CHECK(protected_headers_serialized);
  return *protected_headers_serialized;
}
}  // namespace

CardanoCip30Serializer::RestoredTransactionInput::RestoredTransactionInput() =
    default;
CardanoCip30Serializer::RestoredTransactionInput::~RestoredTransactionInput() =
    default;
CardanoCip30Serializer::RestoredTransactionInput::RestoredTransactionInput(
    const CardanoCip30Serializer::RestoredTransactionInput&) = default;
CardanoCip30Serializer::RestoredTransactionInput&
CardanoCip30Serializer::RestoredTransactionInput::operator=(
    const CardanoCip30Serializer::RestoredTransactionInput&) = default;
CardanoCip30Serializer::RestoredTransactionInput::RestoredTransactionInput(
    CardanoCip30Serializer::RestoredTransactionInput&&) = default;
CardanoCip30Serializer::RestoredTransactionInput&
CardanoCip30Serializer::RestoredTransactionInput::operator=(
    CardanoCip30Serializer::RestoredTransactionInput&&) = default;

CardanoCip30Serializer::RestoredTransactionOutput::RestoredTransactionOutput() =
    default;
CardanoCip30Serializer::RestoredTransactionOutput::
    ~RestoredTransactionOutput() = default;

CardanoCip30Serializer::RestoredTransactionBody::RestoredTransactionBody() =
    default;
CardanoCip30Serializer::RestoredTransactionBody::~RestoredTransactionBody() =
    default;
CardanoCip30Serializer::RestoredTransactionBody::RestoredTransactionBody(
    const RestoredTransactionBody&) = default;
CardanoCip30Serializer::RestoredTransactionBody&
CardanoCip30Serializer::RestoredTransactionBody::operator=(
    const RestoredTransactionBody&) = default;
CardanoCip30Serializer::RestoredTransactionBody::RestoredTransactionBody(
    RestoredTransactionBody&&) = default;
CardanoCip30Serializer::RestoredTransactionBody&
CardanoCip30Serializer::RestoredTransactionBody::operator=(
    RestoredTransactionBody&&) = default;

CardanoCip30Serializer::RestoredTransaction::RestoredTransaction() = default;
CardanoCip30Serializer::RestoredTransaction::~RestoredTransaction() = default;
CardanoCip30Serializer::RestoredTransaction::RestoredTransaction(
    const RestoredTransaction&) = default;
CardanoCip30Serializer::RestoredTransaction&
CardanoCip30Serializer::RestoredTransaction::operator=(
    const RestoredTransaction&) = default;
CardanoCip30Serializer::RestoredTransaction::RestoredTransaction(
    RestoredTransaction&&) = default;
CardanoCip30Serializer::RestoredTransaction&
CardanoCip30Serializer::RestoredTransaction::operator=(RestoredTransaction&&) =
    default;

CardanoCip30Serializer::CardanoCip30Serializer() = default;
CardanoCip30Serializer::~CardanoCip30Serializer() = default;

// static
std::vector<uint8_t> CardanoCip30Serializer::SerializedSignPayload(
    const CardanoAddress& payment_address,
    base::span<const uint8_t> message) {
  // https://github.com/cardano-foundation/CIPs/blob/master/CIP-0008/README.md#signing-and-verification-target-format
  constexpr char kContextSignature1[] = "Signature1";

  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0008#signing-and-verification-target-format
  cbor::Value::ArrayValue sign_payload;
  sign_payload.emplace_back(kContextSignature1);
  sign_payload.emplace_back(MakeSerializedProtectedHeaders(payment_address));
  sign_payload.emplace_back(std::vector<uint8_t>());  // external_aad
  sign_payload.emplace_back(message);

  auto sign_payload_serialized =
      cbor::Writer::Write(cbor::Value(std::move(sign_payload)));
  CHECK(sign_payload_serialized);
  return *sign_payload_serialized;
}

// static
std::vector<uint8_t> CardanoCip30Serializer::SerializeSignedDataKey(
    const CardanoAddress& payment_address,
    base::span<const uint8_t> pubkey) {
  // https://datatracker.ietf.org/doc/html/rfc8152#section-7.1
  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
  constexpr int kKtyKey = 1;
  constexpr int kKtyOKPValue = 1;
  constexpr int kKidKey = 2;
  constexpr int kAlgKey = 3;
  constexpr int kAlgValueEdDSA = -8;
  constexpr int kCrvKey = -1;
  constexpr int kCrvValue = 6;
  constexpr int kXKey = -2;

  cbor::Value::MapValue cose_key;
  cose_key.emplace(kKtyKey, kKtyOKPValue);
  cose_key.emplace(kKidKey, payment_address.ToCborBytes());
  cose_key.emplace(kAlgKey, kAlgValueEdDSA);
  cose_key.emplace(kCrvKey, kCrvValue);
  cose_key.emplace(kXKey, pubkey);
  auto cose_key_serialized =
      cbor::Writer::Write(cbor::Value(std::move(cose_key)));
  CHECK(cose_key_serialized);
  return *cose_key_serialized;
}

// static
std::vector<uint8_t> CardanoCip30Serializer::SerializeSignedDataSignature(
    const CardanoAddress& payment_address,
    base::span<const uint8_t> message,
    base::span<const uint8_t> signature) {
  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
  // https://github.com/cardano-foundation/CIPs/blob/master/CIP-0008/README.md#payload-encoding
  constexpr char kHashedHeaderKey[] = "hashed";
  cbor::Value::MapValue unprotected_headers;
  unprotected_headers.emplace(kHashedHeaderKey, false);

  cbor::Value::ArrayValue cose_sign;
  cose_sign.emplace_back(MakeSerializedProtectedHeaders(payment_address));
  cose_sign.emplace_back(unprotected_headers);
  cose_sign.emplace_back(message);
  cose_sign.emplace_back(signature);
  auto cose_sign_serialized =
      cbor::Writer::Write(cbor::Value(std::move(cose_sign)));
  CHECK(cose_sign_serialized);
  return *cose_sign_serialized;
}

// static
std::string CardanoCip30Serializer::SerializeAmount(uint64_t amount) {
  auto amount_serialized =
      cbor::Writer::Write(cbor::Value(static_cast<int64_t>(amount)));
  CHECK(amount_serialized);
  return HexEncodeLower(*amount_serialized);
}

// static
std::optional<uint64_t> CardanoCip30Serializer::DeserializeAmount(
    const std::string& amount_cbor) {
  std::vector<uint8_t> amount_bytes;
  if (!base::HexStringToBytes(amount_cbor, &amount_bytes)) {
    return std::nullopt;
  }

  auto as_cbor = cbor::Reader::Read(amount_bytes);

  if (!as_cbor) {
    return std::nullopt;
  }

  if (!as_cbor->is_integer()) {
    return std::nullopt;
  }

  return static_cast<uint64_t>(as_cbor->GetInteger());
}

// static
std::vector<std::string> CardanoCip30Serializer::SerializeUtxos(
    const std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>&
        utxos) {
  std::vector<std::string> serialized_utxos;
  for (const auto& item : utxos) {
    cbor::Value::ArrayValue cbor_utxo;

    {
      cbor::Value::ArrayValue tx_input;
      tx_input.emplace_back(item.second.tx_hash);
      tx_input.emplace_back(static_cast<int64_t>(item.second.output_index));
      cbor_utxo.emplace_back(tx_input);
    }

    {
      cbor::Value::ArrayValue tx_output;
      tx_output.emplace_back(item.first.ToCborBytes());
      tx_output.emplace_back(static_cast<int64_t>(item.second.lovelace_amount));
      cbor_utxo.emplace_back(tx_output);
    }

    auto cbor_utxo_serialized =
        cbor::Writer::Write(cbor::Value(std::move(cbor_utxo)));
    CHECK(cbor_utxo_serialized);

    serialized_utxos.push_back(HexEncodeLower(*cbor_utxo_serialized));
  }
  return serialized_utxos;
}

// static
// Deserializes a Cardano transaction from a byte vector (CBOR format).
std::optional<CardanoCip30Serializer::RestoredTransaction>
CardanoCip30Serializer::DeserializeTransaction(
    base::span<const uint8_t> bytes) {
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

  result.raw_bytes = base::ToVector(bytes);
  result.tx_body = std::move(tx_body.value());

  return result;
}

// static
std::optional<std::vector<CardanoCip30Serializer::RestoredTransactionInput>>
CardanoCip30Serializer::DeserializeInputs(const cbor::Value::ArrayValue& data) {
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
    base::CheckedNumeric<uint32_t> index = value.GetArray()[1].GetInteger();
    if (!index.IsValid()) {
      return std::nullopt;
    }
    tx_input.index = index.ValueOrDie();

    tx_inputs.push_back(std::move(tx_input));
  }
  return tx_inputs;
}

// static
std::optional<std::vector<CardanoCip30Serializer::RestoredTransactionOutput>>
CardanoCip30Serializer::DeserializeOutputs(
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

// static
std::optional<std::vector<CardanoCip30Serializer::InputWitness>>
CardanoCip30Serializer::DeserializeWitnessSet(
    const cbor::Value::ArrayValue& data) {
  std::vector<CardanoCip30Serializer::InputWitness> witness_set;
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

// static
std::optional<CardanoCip30Serializer::RestoredTransactionBody>
CardanoCip30Serializer::DeserializeTxBody(const cbor::Value::MapValue& data) {
  auto map_iterator = data.find(cbor::Value(0));

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
  result.inputs = std::move(inputs.value());
  result.outputs = std::move(outputs.value());

  return result;
}

// static
std::optional<std::vector<uint8_t>> CardanoCip30Serializer::ApplySignResults(
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
