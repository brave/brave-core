/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v6_zcash_serializer.h"

#include <algorithm>
#include <array>
#include <map>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"

namespace brave_wallet {

namespace {

// https://github.com/zcash/librustzcash
// components/zcash_protocol/src/constants.rs
// V6_TX_VERSION = 6 | overwintered bit
constexpr uint32_t kV6TxVersion = 6 | 1 << 31;  // 0x80000006
// V6_VERSION_GROUP_ID
constexpr uint32_t kV6VersionGroupId = 0xD884B698;

// v6 digest personalizers differ from v5. Per ZIP 229 / ZIP 246 and confirmed
// against the orchard crate the FFI uses
// (orchard-v0_15/src/bundle/commitments.rs): the v6 Orchard-pool bundle digest
// is personalized with "ZTxIdOrchardH_v6" (NOT the v5 "ZTxIdOrchardHash"), and
// the Ironwood pool uses its own distinct "ZTxIdIronwd_H_v6". These strings
// MUST byte-match what the FFI emits for non-empty bundles, otherwise the
// empty-pool fallback digest won't line up with a non-empty one on the wire.
constexpr char kOrchardV6HashPersonalizer[] = "ZTxIdOrchardH_v6";
constexpr char kIronwoodV6HashPersonalizer[] = "ZTxIdIronwd_H_v6";
constexpr char kSaplingHashPersonalizer[] = "ZTxIdSaplingHash";

void CheckNoTransparentInputs(const ZCashTransaction& tx) {
  CHECK(tx.transparent_part().inputs.empty());
}

}  // namespace

// static
void ZCashV6Serializer::PushHeader(const ZCashTransaction& tx,
                                   BtcLikeSerializerStream& stream) {
  stream.Push32(kV6TxVersion);
  stream.Push32(kV6VersionGroupId);
  stream.Push32(tx.consensus_brach_id());
  stream.Push32(tx.locktime());
  stream.Push32(tx.expiry_height());
  // NOTE: The v6 header ends at expiryHeight. Per ZIP 229 ("Version 6
  // Transaction Format", Non-requirements) the v6 format does NOT carry the
  // `zip233Amount` field (it appeared only in the withdrawn ZIP 230). There
  // are no bytes between nExpiryHeight and tx_in_count. Writing an extra u64
  // here shifts the whole transaction by 8 bytes and makes the node parse a
  // completely empty tx (all count fields read as 0), which it rejects with
  // "must have at least one input". Do not add fields after expiryHeight.
}

// static
std::array<uint8_t, kZCashDigestSize> ZCashV6Serializer::HashHeader(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  PushHeader(tx, stream);
  return ZCashSerializerUtils::Blake2b256(
      stream.data(), base::byte_span_from_cstring("ZTxIdHeadersHash"));
}

// static
// ZIP 246 txid digest (mirrors ZIP 244 with extended header + ironwood node).
std::array<uint8_t, kZCashDigestSize> ZCashV6Serializer::CalculateTxIdDigest(
    const ZCashTransaction& tx) {
  CheckNoTransparentInputs(tx);
  const auto& v6 = tx.v6_part();

  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializerUtils::HashTransparentTxId(tx);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  // Legacy-orchard digest: from FFI if populated, else empty-bundle hash.
  // v6 uses "ZTxIdOrchardH_v6" (not v5's "ZTxIdOrchardHash").
  std::array<uint8_t, kZCashDigestSize> legacy_orchard_hash =
      v6.legacy_orchard.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardV6HashPersonalizer)));
  // Ironwood digest: DISTINCT v6 personalizer "ZTxIdIronwd_H_v6" — the
  // Ironwood pool is domain-separated from Orchard (see orchard
  // commitments.rs).
  std::array<uint8_t, kZCashDigestSize> ironwood_hash =
      v6.ironwood.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kIronwoodV6HashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(legacy_orchard_hash);
  stream.PushBytes(ironwood_hash);

  auto digest_hash = ZCashSerializerUtils::Blake2b256(
      stream.data(), ZCashSerializerUtils::GetHashPersonalizer(tx));
  std::reverse(digest_hash.begin(), digest_hash.end());
  return digest_hash;
}

// static
std::array<uint8_t, kZCashDigestSize>
ZCashV6Serializer::CalculateSignatureDigest(
    const ZCashTransaction& tx,
    const std::optional<ZCashTransaction::TxInput>& input) {
  CheckNoTransparentInputs(tx);
  const auto& v6 = tx.v6_part();

  std::array<uint8_t, kZCashDigestSize> header_hash = HashHeader(tx);
  std::array<uint8_t, kZCashDigestSize> transparent_hash =
      ZCashSerializerUtils::HashTransparentSignature(tx, input);
  std::array<uint8_t, kZCashDigestSize> sapling_hash =
      ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kSaplingHashPersonalizer));
  // Empty-pool fallbacks use the v6 personalizers (see CalculateTxIdDigest).
  std::array<uint8_t, kZCashDigestSize> legacy_orchard_hash =
      v6.legacy_orchard.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kOrchardV6HashPersonalizer)));
  std::array<uint8_t, kZCashDigestSize> ironwood_hash =
      v6.ironwood.digest.value_or(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring(kIronwoodV6HashPersonalizer)));

  BtcLikeSerializerStream stream;
  stream.PushBytes(header_hash);
  stream.PushBytes(transparent_hash);
  stream.PushBytes(sapling_hash);
  stream.PushBytes(legacy_orchard_hash);
  stream.PushBytes(ironwood_hash);

  return ZCashSerializerUtils::Blake2b256(
      stream.data(), ZCashSerializerUtils::GetHashPersonalizer(tx));
}

// static
// Per upstream librustzcash Transaction::write_v6.
std::vector<uint8_t> ZCashV6Serializer::SerializeRawTransaction(
    const ZCashTransaction& tx) {
  CheckNoTransparentInputs(tx);
  const auto& v6 = tx.v6_part();

  BtcLikeSerializerStream stream;

  PushHeader(tx, stream);
  ZCashSerializerUtils::SerializeTransparentInputs(tx, stream);
  ZCashSerializerUtils::SerializeTransparentOutputs(tx, stream);

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

  // NOTE: issuance bundle + memo bundle intentionally omitted (future work).

  return std::move(stream).Take();
}

// static
bool ZCashV6Serializer::SignTransparentPart(
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction& tx) {
  CheckNoTransparentInputs(tx);
  auto addresses = keyring_service.GetZCashAddresses(account_id);
  if (!addresses || addresses->empty()) {
    return false;
  }

  std::map<std::string, mojom::ZCashKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    address_map.emplace(std::move(addr->address_string),
                        std::move(addr->key_id));
  }

  for (auto& input : tx.transparent_part().inputs) {
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
