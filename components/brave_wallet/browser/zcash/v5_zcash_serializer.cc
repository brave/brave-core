/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"

#include <array>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

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
  return ZCashSerializer::Blake2b256(
      stream.data(), base::byte_span_from_cstring("ZTxIdHeadersHash"));
}

// static
// https://zips.z.cash/zip-0244#txid-digest
std::array<uint8_t, kZCashDigestSize> ZCashV5Serializer::CalculateTxIdDigest(
    const ZCashTransaction& tx) {
  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializer::HashTransparentTxId(tx);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  std::array<uint8_t, kZCashDigestSize> orchard_hash =
      tx.orchard_part().digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(orchard_hash);

  auto digest_hash = ZCashSerializer::Blake2b256(
      stream.data(), ZCashSerializer::GetHashPersonalizer(tx));
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
      ZCashSerializer::HashTransparentSignature(tx, input);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  std::array<uint8_t, kZCashDigestSize> orchard_hash =
      tx.orchard_part().digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(orchard_hash);

  return ZCashSerializer::Blake2b256(stream.data(),
                                     ZCashSerializer::GetHashPersonalizer(tx));
}

// static
// https://zips.z.cash/zip-0225
std::vector<uint8_t> ZCashV5Serializer::SerializeRawTransaction(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;

  PushHeader(tx, stream);
  ZCashSerializer::SerializeTransparentInputs(tx, stream);
  ZCashSerializer::SerializeTransparentOutputs(tx, stream);

  // Sapling (empty)
  stream.PushCompactSize(0u);
  stream.PushCompactSize(0u);

  // Orchard
  if (tx.orchard_part().raw_tx) {
    stream.PushBytes(*tx.orchard_part().raw_tx);
  } else {
    stream.PushCompactSize(uint8_t{0});
  }

  return std::move(stream).Take();
}

// static
bool ZCashV5Serializer::SignTransparentPart(
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction& tx) {
  return ZCashSerializer::SignTransparentPart(
      keyring_service, account_id, tx,
      [](const ZCashTransaction& t, const ZCashTransaction::TxInput& in) {
        return ZCashV5Serializer::CalculateSignatureDigest(t, in);
      });
}

}  // namespace brave_wallet
