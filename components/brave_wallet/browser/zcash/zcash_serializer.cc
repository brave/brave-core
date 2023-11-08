/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <string>

#include "base/big_endian.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/common/serializer_stream.h"
#include "brave/third_party/argon2/src/src/blake2/blake2.h"

namespace brave_wallet {

namespace {

constexpr char kTransparentHashPersonalizer[] = "ZTxIdTranspaHash";
constexpr char kSaplingHashPersonalizer[] = "ZTxIdSaplingHash";
constexpr char kOrchardHashPersonalizer[] = "ZTxIdOrchardHash";

constexpr uint32_t kV5TxVersion = 5 | 1 << 31 /* overwintered bit */;
constexpr uint32_t kV5VersionGroupId = 0x26A7270A;
constexpr uint32_t kConsensusBranchId = 0xC2D6D0B4;

std::string GetTxHashPersonalizer() {
  uint32_t consensus = base::ByteSwapToLE32(kConsensusBranchId);
  std::string personalizer("ZcashTxHash_");
  personalizer.append(reinterpret_cast<const char*>(&consensus),
                      sizeof(kConsensusBranchId));
  return personalizer;
}

std::vector<uint8_t> blake2b256(std::vector<uint8_t> payload,
                                std::string personalizer) {
  blake2b_state blakeState;
  blake2b_param params;

  if (personalizer.length() != sizeof(params.personal)) {
    NOTREACHED();
    return {};
  }

  params.digest_length = (uint8_t)32;
  params.key_length = 0;
  params.fanout = 1;
  params.depth = 1;
  params.leaf_length = 0;
  params.node_offset = 0;
  params.node_depth = 0;
  params.inner_length = 0;
  memset(params.reserved, 0, sizeof(params.reserved));
  memset(params.salt, 0, sizeof(params.salt));
  memcpy(params.personal, personalizer.c_str(), sizeof(params.personal));
  if (blake2b_init_param(&blakeState, &params) != 0) {
    NOTREACHED();
    return {};
  }
  if (blake2b_update(&blakeState, payload.data(), payload.size()) != 0) {
    NOTREACHED();
    return {};
  }
  std::vector<uint8_t> result(32, 0);
  if (blake2b_final(&blakeState, result.data(), 32) != 0) {
    NOTREACHED();
    return {};
  }
  return result;
}

void PushHeader(const ZCashTransaction& tx, SerializerStream& stream) {
  stream.Push32AsLE(kV5TxVersion);
  stream.Push32AsLE(kV5VersionGroupId);
  stream.Push32AsLE(kConsensusBranchId);
  stream.Push32AsLE(tx.locktime());
  stream.Push32AsLE(tx.expiry_height());
}

void PushOutpoint(const ZCashTransaction::Outpoint& outpoint,
                  SerializerStream& stream) {
  stream.PushBytes(outpoint.txid);
  stream.Push32AsLE(outpoint.index);
}

void PushOutput(const ZCashTransaction::TxOutput& output,
                SerializerStream& stream) {
  stream.Push64AsLE(output.amount);
  stream.PushSizeAndBytes(output.script_pubkey);
}

std::vector<uint8_t> HashAmounts(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.Push64AsLE(input.utxo_value);
  }
  return blake2b256(data, "ZTxTrAmountsHash");
}

std::vector<uint8_t> HashScriptPubKeys(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.PushSizeAndBytes(input.script_pub_key);
  }
  return blake2b256(data, "ZTxTrScriptsHash");
}

}  // namespace

// static
std::vector<uint8_t> ZCashSerializer::HashTxIn(
    const ZCashTransaction::TxInput& tx_in) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);

  PushOutpoint(tx_in.utxo_outpoint, stream);
  stream.Push64AsLE(tx_in.utxo_value);

  stream.PushSizeAndBytes(tx_in.script_pub_key);

  stream.Push32AsLE(tx_in.n_sequence);
  return blake2b256(data, "Zcash___TxInHash");
}

// static
std::vector<uint8_t> ZCashSerializer::HashPrevouts(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    PushOutpoint(input.utxo_outpoint, stream);
  }
  return blake2b256(data, "ZTxIdPrevoutHash");
}

// static
std::vector<uint8_t> ZCashSerializer::HashSequences(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.Push32AsLE(input.n_sequence);
  }
  return blake2b256(data, "ZTxIdSequencHash");
}

// static
std::vector<uint8_t> ZCashSerializer::HashOutputs(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }
  return blake2b256(data, "ZTxIdOutputsHash");
}

// static
std::vector<uint8_t> ZCashSerializer::HashHeader(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  SerializerStream stream(&data);
  PushHeader(tx, stream);
  return blake2b256(data, "ZTxIdHeadersHash");
}

// static
// https://zips.z.cash/zip-0244#txid-digest
std::vector<uint8_t> ZCashSerializer::CalculateTxIdDigest(
    const ZCashTransaction& zcash_transaction) {
  std::vector<uint8_t> header_hash = HashHeader(zcash_transaction);

  std::vector<uint8_t> transaprent_hash;
  {
    std::vector<uint8_t> data;
    SerializerStream stream(&data);
    stream.PushBytes(ZCashSerializer::HashPrevouts(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashSequences(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashOutputs(zcash_transaction));
    transaprent_hash = blake2b256(data, kTransparentHashPersonalizer);
  }

  std::vector<uint8_t> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::vector<uint8_t> orchard_hash;
  { orchard_hash = blake2b256({}, kOrchardHashPersonalizer); }

  std::vector<uint8_t> digest_hash;
  {
    std::vector<uint8_t> data;
    SerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transaprent_hash);
    stream.PushBytes(sapling_hash);
    stream.PushBytes(orchard_hash);

    digest_hash = blake2b256(data, GetTxHashPersonalizer());
  }

  return digest_hash;
}

// static
// https://zips.z.cash/zip-0244#signature-digest
std::vector<uint8_t> ZCashSerializer::CalculateSignatureDigest(
    const ZCashTransaction& zcash_transaction,
    const ZCashTransaction::TxInput& input) {
  std::vector<uint8_t> header_hash = HashHeader(zcash_transaction);

  std::vector<uint8_t> transaprent_hash;
  {
    std::vector<uint8_t> data;
    SerializerStream stream(&data);
    stream.Push8AsLE(zcash_transaction.sighash_type());
    stream.PushBytes(HashPrevouts(zcash_transaction));
    stream.PushBytes(HashAmounts(zcash_transaction));
    stream.PushBytes(HashScriptPubKeys(zcash_transaction));
    stream.PushBytes(HashSequences(zcash_transaction));
    stream.PushBytes(HashOutputs(zcash_transaction));
    stream.PushBytes(HashTxIn(input));

    transaprent_hash = blake2b256(data, kTransparentHashPersonalizer);
  }

  std::vector<uint8_t> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::vector<uint8_t> orchard_hash;
  { orchard_hash = blake2b256({}, kOrchardHashPersonalizer); }

  std::vector<uint8_t> digest_hash;
  {
    std::vector<uint8_t> data;
    SerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transaprent_hash);
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
  SerializerStream stream(&data);

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
  {
    // Outputs size
    stream.PushVarInt(zcash_transaction.outputs().size());
    for (const auto& output : zcash_transaction.outputs()) {
      PushOutput(output, stream);
    }
  }

  // Sapling
  {
    stream.PushVarInt(0);
    stream.PushVarInt(0);
  }

  // Orchard
  { stream.PushVarInt(0); }

  return data;
}

}  // namespace brave_wallet
