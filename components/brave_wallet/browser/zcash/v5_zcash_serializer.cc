/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"

#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"

namespace brave_wallet {

namespace {

// https://zips.z.cash/zip-0225
constexpr uint32_t kV5TxVersion = 5 | 1 << 31 /* overwintered bit */;
// https://zips.z.cash/protocol/protocol.pdf#txnconsensus
constexpr uint32_t kV5VersionGroupId = 0x26A7270A;

constexpr char kOrchardHashPersonalizer[] = "ZTxIdOrchardHash";
constexpr char kSaplingHashPersonalizer[] = "ZTxIdSaplingHash";

}  // namespace

// static
void ZCashV5Serializer::PushHeader(const ZCashTransaction& tx,
                                   BtcLikeSerializerStream& stream) {
  stream.Push32(kV5TxVersion);
  stream.Push32(kV5VersionGroupId);
  stream.Push32(tx.consensus_brach_id());
  stream.Push32(tx.locktime());
  stream.Push32(tx.expiry_height());
}

// static
// https://zips.z.cash/zip-0244#t-1-header-digest
std::array<uint8_t, kZCashDigestSize> ZCashV5Serializer::HashHeader(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  PushHeader(tx, stream);
  return ZCashSerializerUtils::Blake2b256(
      stream.data(), base::byte_span_from_cstring("ZTxIdHeadersHash"));
}

// static
// https://zips.z.cash/zip-0244#txid-digest
std::array<uint8_t, kZCashDigestSize> ZCashV5Serializer::CalculateTxIdDigest(
    const ZCashTransaction& tx) {
  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializerUtils::HashTransparentTxId(tx);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  std::array<uint8_t, kZCashDigestSize> orchard_hash =
      tx.v5_part().orchard.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(orchard_hash);

  auto digest_hash = ZCashSerializerUtils::Blake2b256(
      stream.data(), ZCashSerializerUtils::GetHashPersonalizer(tx));
  std::reverse(digest_hash.begin(), digest_hash.end());
  return digest_hash;
}

// static
// https://zips.z.cash/zip-0244#signature-digest
std::array<uint8_t, kZCashDigestSize>
ZCashV5Serializer::CalculateSignatureDigest(
    const ZCashTransaction& tx,
    const std::optional<ZCashTransaction::TxInput>& input) {
  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializerUtils::HashTransparentSignature(tx, input);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  std::array<uint8_t, kZCashDigestSize> orchard_hash =
      tx.v5_part().orchard.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(orchard_hash);

  return ZCashSerializerUtils::Blake2b256(
      stream.data(), ZCashSerializerUtils::GetHashPersonalizer(tx));
}

// static
// https://zips.z.cash/zip-0225
std::vector<uint8_t> ZCashV5Serializer::SerializeRawTransaction(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;

  PushHeader(tx, stream);
  ZCashSerializerUtils::SerializeTransparentInputs(tx, stream);
  ZCashSerializerUtils::SerializeTransparentOutputs(tx, stream);

  // Sapling (empty)
  stream.PushCompactSize(0u);
  stream.PushCompactSize(0u);

  // Orchard
  if (tx.v5_part().orchard.raw_tx) {
    stream.PushBytes(*tx.v5_part().orchard.raw_tx);
  } else {
    stream.PushCompactSize(uint8_t{0});
  }

  return std::move(stream).Take();
}

// static
bool ZCashV5Serializer::SignTransparentPartV5(
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction& tx) {
  auto addresses = keyring_service.GetZCashAddresses(account_id);
  if (!addresses || addresses->empty()) {
    return false;
  }

  std::map<std::string, mojom::ZCashKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    address_map.emplace(std::move(addr->address_string),
                        std::move(addr->key_id));
  }

  for (size_t input_index = 0;
       input_index < tx.transparent_part().inputs.size(); ++input_index) {
    auto& input = tx.transparent_part().inputs[input_index];

    if (!address_map.contains(input.utxo_address)) {
      return false;
    }

    auto& key_id = address_map.at(input.utxo_address);

    auto pubkey = keyring_service.GetZCashPubKey(account_id, key_id);
    if (!pubkey) {
      return false;
    }

    auto signature_digest = CalculateSignatureDigest(tx, input);

    auto signature = keyring_service.SignMessageByZCashKeyring(
        account_id, key_id, base::span(signature_digest));

    if (!signature) {
      return false;
    }

    ZCashSerializer::SerializeSignature(tx, input, pubkey.value(),
                                        signature.value());
  }

  return true;
}

}  // namespace brave_wallet
