/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"

#include <array>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"

namespace brave_wallet {

namespace {

// https://zips.z.cash/zip-0244
constexpr char kTransparentHashPersonalizer[] = "ZTxIdTranspaHash";

// https://zips.z.cash/zip-0244#txid-digest-1
constexpr char kTxHashPersonalizerPrefix[] = "ZcashTxHash_";

}  // namespace

// static
std::array<uint8_t, kZCashDigestSize>
ZCashSerializerUtils::CalculateSignatureDigest(
    const ZCashTransaction& zcash_transaction,
    const std::optional<ZCashTransaction::TxInput>& input) {
  return ZCashV5Serializer::CalculateSignatureDigest(zcash_transaction, input);
}

// static
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::Blake2b256(
    base::span<const uint8_t> payload,
    base::span<const uint8_t, kBlake2bPersonalizerLength> personalizer) {
  return Blake2bHash<kZCashDigestSize>({payload}, personalizer);
}

// static
void ZCashSerializerUtils::PushOutpoint(
    const ZCashTransaction::Outpoint& outpoint,
    BtcLikeSerializerStream& stream) {
  stream.PushBytes(outpoint.txid);
  stream.Push32(outpoint.index);
}

// static
void ZCashSerializerUtils::PushOutput(const ZCashTransaction::TxOutput& output,
                                      BtcLikeSerializerStream& stream) {
  stream.Push64(output.amount);
  stream.PushSizeAndBytes(output.script_pubkey);
}

// static
void ZCashSerializerUtils::SerializeTransparentInputs(
    const ZCashTransaction& tx,
    BtcLikeSerializerStream& stream) {
  stream.PushCompactSize(tx.transparent_part().inputs.size());
  for (const auto& input : tx.transparent_part().inputs) {
    PushOutpoint(input.utxo_outpoint, stream);
    stream.PushSizeAndBytes(input.script_sig);
    stream.Push32(input.n_sequence);
  }
}

// static
void ZCashSerializerUtils::SerializeTransparentOutputs(
    const ZCashTransaction& tx,
    BtcLikeSerializerStream& stream) {
  stream.PushCompactSize(tx.transparent_part().outputs.size());
  for (const auto& output : tx.transparent_part().outputs) {
    PushOutput(output, stream);
  }
}

// static
std::array<uint8_t, kBlake2bPersonalizerLength>
ZCashSerializerUtils::GetHashPersonalizer(const ZCashTransaction& tx) {
  std::array<uint8_t, kBlake2bPersonalizerLength> result;
  auto span_writer = base::SpanWriter(base::span(result));
  span_writer.Write(base::byte_span_from_cstring(kTxHashPersonalizerPrefix));
  span_writer.WriteU32LittleEndian(tx.consensus_brach_id());
  DCHECK_EQ(span_writer.remaining(), 0u);
  return result;
}

// static
// https://zips.z.cash/zip-0244#s-2g-txin-sig-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashTxIn(
    std::optional<ZCashTransaction::TxInput> tx_in) {
  BtcLikeSerializerStream stream;
  if (tx_in) {
    PushOutpoint(tx_in->utxo_outpoint, stream);
    stream.Push64(tx_in->utxo_value);
    stream.PushSizeAndBytes(tx_in->script_pub_key);
    stream.Push32(tx_in->n_sequence);
  }
  return Blake2b256(std::move(stream).data(),
                    base::byte_span_from_cstring("Zcash___TxInHash"));
}

// static
// https://zips.z.cash/zip-0244#s-2c-amounts-sig-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashAmounts(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  for (const auto& input : tx.transparent_part().inputs) {
    stream.Push64(input.utxo_value);
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring("ZTxTrAmountsHash"));
}

// static
// https://zips.z.cash/zip-0244#s-2d-scriptpubkeys-sig-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashScriptPubKeys(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  for (const auto& input : tx.transparent_part().inputs) {
    stream.PushSizeAndBytes(input.script_pub_key);
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring("ZTxTrScriptsHash"));
}

// static
// https://zips.z.cash/zip-0244#t-2a-prevouts-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashPrevouts(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  for (const auto& input : tx.transparent_part().inputs) {
    PushOutpoint(input.utxo_outpoint, stream);
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring("ZTxIdPrevoutHash"));
}

// static
// https://zips.z.cash/zip-0244#t-2b-sequence-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashSequences(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  for (const auto& input : tx.transparent_part().inputs) {
    stream.Push32(input.n_sequence);
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring("ZTxIdSequencHash"));
}

// static
// https://zips.z.cash/zip-0244#t-2c-outputs-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashOutputs(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  for (const auto& output : tx.transparent_part().outputs) {
    PushOutput(output, stream);
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring("ZTxIdOutputsHash"));
}

// static
// https://zips.z.cash/zip-0244 T.2 — transparent txid digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializerUtils::HashTransparentTxId(
    const ZCashTransaction& tx) {
  BtcLikeSerializerStream stream;
  if (!tx.transparent_part().inputs.empty() ||
      !tx.transparent_part().outputs.empty()) {
    stream.PushBytes(HashPrevouts(tx));
    stream.PushBytes(HashSequences(tx));
    stream.PushBytes(HashOutputs(tx));
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring(kTransparentHashPersonalizer));
}

// static
// https://zips.z.cash/zip-0244 T.1 — transparent signature digest
std::array<uint8_t, kZCashDigestSize>
ZCashSerializerUtils::HashTransparentSignature(
    const ZCashTransaction& tx,
    const std::optional<ZCashTransaction::TxInput>& input) {
  BtcLikeSerializerStream stream;
  if (!tx.transparent_part().inputs.empty()) {
    stream.Push8(tx.sighash_type());
    stream.PushBytes(HashPrevouts(tx));
    stream.PushBytes(HashAmounts(tx));
    stream.PushBytes(HashScriptPubKeys(tx));
    stream.PushBytes(HashSequences(tx));
    stream.PushBytes(HashOutputs(tx));
    stream.PushBytes(HashTxIn(input));
  } else if (!tx.transparent_part().outputs.empty()) {
    stream.PushBytes(HashPrevouts(tx));
    stream.PushBytes(HashSequences(tx));
    stream.PushBytes(HashOutputs(tx));
  }
  return Blake2b256(stream.data(),
                    base::byte_span_from_cstring(kTransparentHashPersonalizer));
}

}  // namespace brave_wallet
