/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <string>
#include <string_view>

#include "base/big_endian.h"
#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"

namespace brave_wallet {

namespace {

// https://zips.z.cash/zip-0244
constexpr char kTransparentHashPersonalizer[] = "ZTxIdTranspaHash";
constexpr char kSaplingHashPersonalizer[] = "ZTxIdSaplingHash";
constexpr char kOrchardHashPersonalizer[] = "ZTxIdOrchardHash";
constexpr char kTxHashPersonalizerPrefix[] = "ZcashTxHash_";

constexpr uint32_t kV5TxVersion = 5 | 1 << 31 /* overwintered bit */;
// https://zips.z.cash/protocol/protocol.pdf#txnconsensus
constexpr uint32_t kV5VersionGroupId = 0x26A7270A;
constexpr uint32_t kConsensusBranchId = 0xC2D6D0B4;

// https://zips.z.cash/zip-0244#txid-digest-1
std::string GetTxHashPersonalizer() {
  std::string personalizer(kTxHashPersonalizerPrefix);

  personalizer.append(sizeof(kConsensusBranchId), '\0');
  base::as_writable_byte_span(personalizer)
      .subspan<std::string(kTxHashPersonalizerPrefix).size(),
               sizeof(kConsensusBranchId)>()
      .copy_from(base::byte_span_from_ref(base::numerics::U32FromLittleEndian(
          base::byte_span_from_ref(kConsensusBranchId))));

  return personalizer;
}

std::array<uint8_t, 32> blake2b256(const std::vector<uint8_t>& payload,
                                   std::string_view personalizer) {
  blake2b_state blake_state = {};
  blake2b_param params = {};

  if (personalizer.length() != sizeof(params.personal)) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }

  params.digest_length = 32;
  params.fanout = 1;
  params.depth = 1;
  memcpy(params.personal, personalizer.data(), sizeof(params.personal));
  if (blake2b_init_param(&blake_state, &params) != 0) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }
  if (blake2b_update(&blake_state, payload.data(), payload.size()) != 0) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }
  std::array<uint8_t, 32> result;
  if (blake2b_final(&blake_state, result.data(), 32) != 0) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }

  return result;
}

void PushHeader(const ZCashTransaction& tx, BtcLikeSerializerStream& stream) {
  stream.Push32AsLE(kV5TxVersion);
  stream.Push32AsLE(kV5VersionGroupId);
  stream.Push32AsLE(kConsensusBranchId);
  stream.Push32AsLE(tx.locktime());
  stream.Push32AsLE(tx.expiry_height());
}

void PushOutpoint(const ZCashTransaction::Outpoint& outpoint,
                  BtcLikeSerializerStream& stream) {
  stream.PushBytes(outpoint.txid);
  stream.Push32AsLE(outpoint.index);
}

void PushOutput(const ZCashTransaction::TxOutput& output,
                BtcLikeSerializerStream& stream) {
  stream.Push64AsLE(output.amount);
  stream.PushSizeAndBytes(output.script_pubkey);
}

// https://zips.z.cash/zip-0244#s-2c-amounts-sig-digest
std::array<uint8_t, 32> HashAmounts(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.Push64AsLE(input.utxo_value);
  }
  return blake2b256(data, "ZTxTrAmountsHash");
}

// https://zips.z.cash/zip-0244#s-2d-scriptpubkeys-sig-digest
std::array<uint8_t, 32> HashScriptPubKeys(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.PushSizeAndBytes(input.script_pub_key);
  }
  return blake2b256(data, "ZTxTrScriptsHash");
}

}  // namespace

// static
// https://zips.z.cash/zip-0244#s-2g-txin-sig-digest
std::array<uint8_t, 32> ZCashSerializer::HashTxIn(
    const ZCashTransaction::TxInput& tx_in) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);

  PushOutpoint(tx_in.utxo_outpoint, stream);
  stream.Push64AsLE(tx_in.utxo_value);

  stream.PushSizeAndBytes(tx_in.script_pub_key);

  stream.Push32AsLE(tx_in.n_sequence);
  return blake2b256(data, "Zcash___TxInHash");
}

// static
// https://zips.z.cash/zip-0244#t-2a-prevouts-digest
std::array<uint8_t, 32> ZCashSerializer::HashPrevouts(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    PushOutpoint(input.utxo_outpoint, stream);
  }
  return blake2b256(data, "ZTxIdPrevoutHash");
}

// static
// https://zips.z.cash/zip-0244#t-2b-sequence-digest
std::array<uint8_t, 32> ZCashSerializer::HashSequences(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.Push32AsLE(input.n_sequence);
  }
  return blake2b256(data, "ZTxIdSequencHash");
}

// static
// https://zips.z.cash/zip-0244#t-2a-prevouts-digest
std::array<uint8_t, 32> ZCashSerializer::HashOutputs(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }
  return blake2b256(data, "ZTxIdOutputsHash");
}

// static
// https://zips.z.cash/zip-0244#t-1-header-digest
std::array<uint8_t, 32> ZCashSerializer::HashHeader(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  PushHeader(tx, stream);
  return blake2b256(data, "ZTxIdHeadersHash");
}

// static
// https://zips.z.cash/zip-0244#txid-digest
std::array<uint8_t, 32> ZCashSerializer::CalculateTxIdDigest(
    const ZCashTransaction& zcash_transaction) {
  std::array<uint8_t, 32> header_hash = HashHeader(zcash_transaction);

  std::array<uint8_t, 32> transparent_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(ZCashSerializer::HashPrevouts(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashSequences(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashOutputs(zcash_transaction));
    transparent_hash = blake2b256(data, kTransparentHashPersonalizer);
  }

  std::array<uint8_t, 32> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::array<uint8_t, 32> orchard_hash;
  { orchard_hash = blake2b256({}, kOrchardHashPersonalizer); }

  std::array<uint8_t, 32> digest_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transparent_hash);
    stream.PushBytes(sapling_hash);
    stream.PushBytes(orchard_hash);

    digest_hash = blake2b256(data, GetTxHashPersonalizer());
  }

  std::reverse(digest_hash.begin(), digest_hash.end());

  return digest_hash;
}

// static
// https://zips.z.cash/zip-0244#signature-digest
std::array<uint8_t, 32> ZCashSerializer::CalculateSignatureDigest(
    const ZCashTransaction& zcash_transaction,
    const ZCashTransaction::TxInput& input) {
  std::array<uint8_t, 32> header_hash = HashHeader(zcash_transaction);

  std::array<uint8_t, 32> transparent_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.Push8AsLE(zcash_transaction.sighash_type());
    stream.PushBytes(HashPrevouts(zcash_transaction));
    stream.PushBytes(HashAmounts(zcash_transaction));
    stream.PushBytes(HashScriptPubKeys(zcash_transaction));
    stream.PushBytes(HashSequences(zcash_transaction));
    stream.PushBytes(HashOutputs(zcash_transaction));
    stream.PushBytes(HashTxIn(input));

    transparent_hash = blake2b256(data, kTransparentHashPersonalizer);
  }

  std::array<uint8_t, 32> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::array<uint8_t, 32> orchard_hash;
  { orchard_hash = blake2b256({}, kOrchardHashPersonalizer); }

  std::array<uint8_t, 32> digest_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transparent_hash);
    stream.PushBytes(sapling_hash);
    stream.PushBytes(orchard_hash);

    digest_hash = blake2b256(data, GetTxHashPersonalizer());
  }

  return digest_hash;
}

// static
// https://zips.z.cash/zip-0225
std::vector<uint8_t> ZCashSerializer::SerializeRawTransaction(
    const ZCashTransaction& zcash_transaction) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);

  PushHeader(zcash_transaction, stream);

  // Tx In
  {
    // Inputs size
    stream.PushVarInt(zcash_transaction.inputs().size());
    for (const auto& input : zcash_transaction.inputs()) {
      // Outpoint
      PushOutpoint(input.utxo_outpoint, stream);
      stream.PushSizeAndBytes(input.script_sig);
      // Sequence
      stream.Push32AsLE(input.n_sequence);
    }
  }

  // Tx Out

  // Outputs size
  stream.PushVarInt(zcash_transaction.outputs().size());
  for (const auto& output : zcash_transaction.outputs()) {
    PushOutput(output, stream);
  }

  // Sapling
  stream.PushVarInt(0);
  stream.PushVarInt(0);

  // Orchard
  stream.PushVarInt(0);

  return data;
}

}  // namespace brave_wallet
