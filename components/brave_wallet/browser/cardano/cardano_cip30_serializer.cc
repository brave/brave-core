/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

#include <cstdint>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
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
std::optional<std::string> CardanoCip30Serializer::SerializeAmount(
    uint64_t amount) {
  // Chromium cbor does not support uint64_t, but only int64_t.
  int64_t amount_to_serialize = 0;
  if (!base::CheckedNumeric(amount).AssignIfValid(&amount_to_serialize)) {
    return std::nullopt;
  }
  auto amount_serialized =
      cbor::Writer::Write(cbor::Value(amount_to_serialize));
  CHECK(amount_serialized);
  return base::HexEncodeLower(*amount_serialized);
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

  if (!as_cbor->is_integer() || !as_cbor->is_unsigned()) {
    return std::nullopt;
  }

  return base::CheckedNumeric<uint64_t>(as_cbor->GetUnsigned()).ValueOrDie();
}

// static
std::optional<std::vector<std::string>> CardanoCip30Serializer::SerializeUtxos(
    base::span<const cardano_rpc::UnspentOutput> utxos) {
  std::vector<std::string> serialized_utxos;
  for (const auto& item : utxos) {
    cbor::Value::ArrayValue cbor_utxo;

    {
      cbor::Value::ArrayValue tx_input;
      tx_input.emplace_back(item.tx_hash);
      tx_input.emplace_back(base::strict_cast<int64_t>(item.output_index));
      cbor_utxo.emplace_back(tx_input);
    }

    {
      cbor::Value::ArrayValue tx_output;
      tx_output.emplace_back(item.address_to.ToCborBytes());
      int64_t val = 0;
      if (!base::CheckedNumeric(item.lovelace_amount).AssignIfValid(&val)) {
        return std::nullopt;
      }
      tx_output.emplace_back(val);
      cbor_utxo.emplace_back(tx_output);
    }

    auto cbor_utxo_serialized =
        cbor::Writer::Write(cbor::Value(std::move(cbor_utxo)));
    CHECK(cbor_utxo_serialized);

    serialized_utxos.push_back(base::HexEncodeLower(*cbor_utxo_serialized));
  }
  return serialized_utxos;
}

}  // namespace brave_wallet
