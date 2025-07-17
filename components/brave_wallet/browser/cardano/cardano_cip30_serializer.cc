/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

#include <cstdint>
#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "components/cbor/values.h"
#include "components/cbor/writer.h"

namespace brave_wallet {

namespace {
std::vector<uint8_t> MakeSerializedProtectedHeaders(
    const CardanoAddress& payment_address) {
  // https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
  constexpr int kAlgHeaderKey = 1;
  constexpr int kAlgHeaderValueEdDSA = -8;
  constexpr int kKidHeaderKey = -4;
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

}  // namespace brave_wallet
