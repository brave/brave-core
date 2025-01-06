/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "base/big_endian.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/numerics/byte_conversions.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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

std::array<uint8_t, kZCashDigestSize> blake2b256(
    const std::vector<uint8_t>& payload,
    std::string_view personalizer) {
  blake2b_state blake_state = {};
  blake2b_param params = {};

  if (personalizer.length() != sizeof(params.personal)) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }

  params.digest_length = kZCashDigestSize;
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
  std::array<uint8_t, kZCashDigestSize> result;
  if (blake2b_final(&blake_state, result.data(), kZCashDigestSize) != 0) {
    NOTREACHED_IN_MIGRATION();
    return {};
  }

  return result;
}

void PushHeader(const ZCashTransaction& tx, BtcLikeSerializerStream& stream) {
  stream.Push32AsLE(kV5TxVersion);
  stream.Push32AsLE(kV5VersionGroupId);
  stream.Push32AsLE(tx.consensus_brach_id());
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
  for (const auto& input : tx.transparent_part().inputs) {
    stream.Push64AsLE(input.utxo_value);
  }
  return blake2b256(data, "ZTxTrAmountsHash");
}

// https://zips.z.cash/zip-0244#s-2d-scriptpubkeys-sig-digest
std::array<uint8_t, 32> HashScriptPubKeys(const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.transparent_part().inputs) {
    stream.PushSizeAndBytes(input.script_pub_key);
  }
  return blake2b256(data, "ZTxTrScriptsHash");
}

std::string GetHashPersonalizer(const ZCashTransaction& tx) {
  std::string personalizer(kTxHashPersonalizerPrefix);

  personalizer.append(sizeof(tx.consensus_brach_id()), '\0');
  base::as_writable_byte_span(personalizer)
      .subspan<std::string(kTxHashPersonalizerPrefix).size(),
               sizeof(tx.consensus_brach_id())>()
      .copy_from(base::byte_span_from_ref(base::numerics::U32FromLittleEndian(
          base::byte_span_from_ref(tx.consensus_brach_id()))));

  return personalizer;
}

}  // namespace

// static
void ZCashSerializer::SerializeSignature(
    const ZCashTransaction& tx,
    ZCashTransaction::TxInput& input,
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& signature) {
  DCHECK(input.script_sig.empty());
  BtcLikeSerializerStream stream(&input.script_sig);
  stream.PushVarInt(signature.size() + 1);
  stream.PushBytes(signature);
  stream.Push8AsLE(tx.sighash_type());
  stream.PushSizeAndBytes(pubkey);
}

// static
// https://zips.z.cash/zip-0244#s-2g-txin-sig-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashTxIn(
    std::optional<ZCashTransaction::TxInput> tx_in) {
  std::vector<uint8_t> data;
  if (tx_in) {
    BtcLikeSerializerStream stream(&data);

    PushOutpoint(tx_in->utxo_outpoint, stream);
    stream.Push64AsLE(tx_in->utxo_value);

    stream.PushSizeAndBytes(tx_in->script_pub_key);

    stream.Push32AsLE(tx_in->n_sequence);
  }

  return blake2b256(data, "Zcash___TxInHash");
}

// static
bool ZCashSerializer::SignTransparentPart(KeyringService* keyring_service,
                                          const mojom::AccountIdPtr& account_id,
                                          ZCashTransaction& tx) {
  auto addresses = keyring_service->GetZCashAddresses(account_id);
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

    auto pubkey = keyring_service->GetZCashPubKey(account_id, key_id);
    if (!pubkey) {
      return false;
    }

    auto signature_digest =
        ZCashSerializer::CalculateSignatureDigest(tx, input);

    auto signature = keyring_service->SignMessageByZCashKeyring(
        account_id, key_id, base::span(signature_digest));

    if (!signature) {
      return false;
    }

    SerializeSignature(tx, input, pubkey.value(), signature.value());
  }

  return true;
}

// static
// https://zips.z.cash/zip-0244#t-2a-prevouts-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashPrevouts(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.transparent_part().inputs) {
    PushOutpoint(input.utxo_outpoint, stream);
  }
  return blake2b256(data, "ZTxIdPrevoutHash");
}

// static
// https://zips.z.cash/zip-0244#t-2b-sequence-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashSequences(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.transparent_part().inputs) {
    stream.Push32AsLE(input.n_sequence);
  }
  return blake2b256(data, "ZTxIdSequencHash");
}

// static
// https://zips.z.cash/zip-0244#t-2a-prevouts-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashOutputs(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& output : tx.transparent_part().outputs) {
    PushOutput(output, stream);
  }
  return blake2b256(data, "ZTxIdOutputsHash");
}

// static
// https://zips.z.cash/zip-0244#t-1-header-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashHeader(
    const ZCashTransaction& tx) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  PushHeader(tx, stream);
  return blake2b256(data, "ZTxIdHeadersHash");
}

// static
// https://zips.z.cash/zip-0244#txid-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::CalculateTxIdDigest(
    const ZCashTransaction& zcash_transaction) {
  std::array<uint8_t, kZCashDigestSize> header_hash =
      HashHeader(zcash_transaction);

  std::array<uint8_t, kZCashDigestSize> transparent_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(ZCashSerializer::HashPrevouts(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashSequences(zcash_transaction));
    stream.PushBytes(ZCashSerializer::HashOutputs(zcash_transaction));
    transparent_hash = blake2b256(data, kTransparentHashPersonalizer);
  }

  std::array<uint8_t, kZCashDigestSize> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::array<uint8_t, kZCashDigestSize> orchard_hash;
  {
    orchard_hash = zcash_transaction.orchard_part().digest.value_or(
        blake2b256(std::vector<uint8_t>(), kOrchardHashPersonalizer));
  }

  std::array<uint8_t, kZCashDigestSize> digest_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transparent_hash);
    stream.PushBytes(sapling_hash);
    stream.PushBytes(orchard_hash);

    digest_hash = blake2b256(data, GetHashPersonalizer(zcash_transaction));
  }

  std::reverse(digest_hash.begin(), digest_hash.end());

  return digest_hash;
}

// static
// https://zips.z.cash/zip-0244#signature-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::CalculateSignatureDigest(
    const ZCashTransaction& zcash_transaction,
    const std::optional<ZCashTransaction::TxInput>& input) {
  std::array<uint8_t, kZCashDigestSize> header_hash =
      HashHeader(zcash_transaction);

  std::array<uint8_t, kZCashDigestSize> transparent_hash;
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

  std::array<uint8_t, kZCashDigestSize> sapling_hash;
  { sapling_hash = blake2b256({}, kSaplingHashPersonalizer); }

  std::array<uint8_t, kZCashDigestSize> orchard_hash;
  {
    orchard_hash = zcash_transaction.orchard_part().digest.value_or(
        blake2b256({}, kOrchardHashPersonalizer));
  }

  std::array<uint8_t, kZCashDigestSize> digest_hash;
  {
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushBytes(header_hash);
    stream.PushBytes(transparent_hash);
    stream.PushBytes(sapling_hash);
    stream.PushBytes(orchard_hash);

    digest_hash = blake2b256(data, GetHashPersonalizer(zcash_transaction));
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
    stream.PushVarInt(zcash_transaction.transparent_part().inputs.size());
    for (const auto& input : zcash_transaction.transparent_part().inputs) {
      // Outpoint
      PushOutpoint(input.utxo_outpoint, stream);
      stream.PushSizeAndBytes(input.script_sig);
      // Sequence
      stream.Push32AsLE(input.n_sequence);
    }
  }

  // Tx Out

  // Outputs size
  stream.PushVarInt(zcash_transaction.transparent_part().outputs.size());
  for (const auto& output : zcash_transaction.transparent_part().outputs) {
    PushOutput(output, stream);
  }

  // Sapling
  stream.PushVarInt(0);
  stream.PushVarInt(0);

  // Orchard
  if (zcash_transaction.orchard_part().raw_tx) {
    stream.PushBytes(zcash_transaction.orchard_part().raw_tx.value());
  } else {
    stream.PushVarInt(0);
  }

  return data;
}

}  // namespace brave_wallet
