/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v6_zcash_serializer.h"

#include <array>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

namespace brave_wallet {

namespace {

// https://github.com/zcash/librustzcash
// components/zcash_protocol/src/constants.rs
// V6_TX_VERSION = 6 | overwintered bit
constexpr uint32_t kV6TxVersion = 6 | 1 << 31;  // 0x80000006
// V6_VERSION_GROUP_ID
constexpr uint32_t kV6VersionGroupId = 0xD884B698;

// NU7 consensus branch id (placeholder; comes from tx.consensus_brach_id()).
// Only reachable behind IsZCashIronwoodTransactionEnabled().

constexpr char kOrchardHashPersonalizer[] = "ZTxIdOrchardHash";
constexpr char kSaplingHashPersonalizer[] = "ZTxIdSaplingHash";

}  // namespace

// static
void ZCashV6Serializer::PushHeader(const ZCashTransaction& tx,
                                   BtcLikeSerializerStream& stream) {
  stream.Push32(kV6TxVersion);
  stream.Push32(kV6VersionGroupId);
  stream.Push32(tx.consensus_brach_id());
  stream.Push32(tx.locktime());
  stream.Push32(tx.expiry_height());
  // ZIP 233: value removed from circulation (u64 LE). NEW vs v5.
  stream.Push64(
      base::checked_cast<uint64_t>(tx.v6_part().zip233_amount));
}

// static
std::array<uint8_t, kZCashDigestSize> ZCashV6Serializer::HashHeader(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  PushHeader(tx, stream);
  return ZCashSerializer::Blake2b256(
      stream.data(), base::byte_span_from_cstring("ZTxIdHeadersHash"));
}

// static
// ZIP 246 txid digest (mirrors ZIP 244 with extended header + ironwood node).
std::array<uint8_t, kZCashDigestSize> ZCashV6Serializer::CalculateTxIdDigest(
    const ZCashTransaction& tx) {
  const auto& v6 = tx.v6_part();

  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializer::HashTransparentTxId(tx);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  // Legacy-orchard digest: from FFI if populated, else empty-bundle hash.
  std::array<uint8_t, kZCashDigestSize> legacy_orchard_hash =
      v6.legacy_orchard.digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));
  // Ironwood digest: same personalizer as legacy-orchard — verified against
  // orchard-v0_14 crate (see i-want-you-to-jazzy-shamir.md Context).
  std::array<uint8_t, kZCashDigestSize> ironwood_hash =
      v6.ironwood.digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(legacy_orchard_hash);
  stream.PushBytes(ironwood_hash);

  auto digest_hash = ZCashSerializer::Blake2b256(
      stream.data(), ZCashSerializer::GetHashPersonalizer(tx));
  std::reverse(digest_hash.begin(), digest_hash.end());
  return digest_hash;
}

// static
std::array<uint8_t, kZCashDigestSize>
ZCashV6Serializer::CalculateSignatureDigest(
    const ZCashTransaction& tx,
    const std::optional<ZCashTransaction::TxInput>& input) {
  const auto& v6 = tx.v6_part();

  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializer::HashTransparentSignature(tx, input);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  std::array<uint8_t, kZCashDigestSize> legacy_orchard_hash =
      v6.legacy_orchard.digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));
  std::array<uint8_t, kZCashDigestSize> ironwood_hash =
      v6.ironwood.digest.value_or(ZCashSerializer::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardHashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(legacy_orchard_hash);
  stream.PushBytes(ironwood_hash);

  return ZCashSerializer::Blake2b256(stream.data(),
                                     ZCashSerializer::GetHashPersonalizer(tx));
}

// static
// Per upstream librustzcash Transaction::write_v6.
std::vector<uint8_t> ZCashV6Serializer::SerializeRawTransaction(
    const ZCashTransaction& tx) {
  const auto& v6 = tx.v6_part();

  BtcLikeSerializerStream stream;

  PushHeader(tx, stream);
  ZCashSerializer::SerializeTransparentInputs(tx, stream);
  ZCashSerializer::SerializeTransparentOutputs(tx, stream);

  // Sapling (empty)
  stream.PushCompactSize(0u);
  stream.PushCompactSize(0u);

  // Legacy-orchard bundle
  if (v6.legacy_orchard.raw_tx) {
    stream.PushBytes(*v6.legacy_orchard.raw_tx);
  } else {
    stream.PushCompactSize(uint8_t{0});
  }

  // Ironwood bundle
  if (v6.ironwood.raw_tx) {
    stream.PushBytes(*v6.ironwood.raw_tx);
  } else {
    stream.PushCompactSize(uint8_t{0});
  }

  // NOTE: issuance bundle + memo bundle intentionally omitted (Phase 7).

  return std::move(stream).Take();
}

// static
bool ZCashV6Serializer::SignTransparentPart(
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction& tx) {
  return ZCashSerializer::SignTransparentPart(
      keyring_service, account_id, tx,
      [](const ZCashTransaction& t, const ZCashTransaction::TxInput& in) {
        return ZCashV6Serializer::CalculateSignatureDigest(t, in);
      });
}

}  // namespace brave_wallet
