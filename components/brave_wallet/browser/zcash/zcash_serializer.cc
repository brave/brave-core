/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <array>
#include <map>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/v6_zcash_serializer.h"

namespace brave_wallet {

namespace {

// https://zips.z.cash/zip-0244
constexpr char kTransparentHashPersonalizer[] = "ZTxIdTranspaHash";

// https://zips.z.cash/zip-0244#txid-digest-1
constexpr char kTxHashPersonalizerPrefix[] = "ZcashTxHash_";

}  // namespace

// static
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::Blake2b256(
    base::span<const uint8_t> payload,
    base::span<const uint8_t, kBlake2bPersonalizerLength> personalizer) {
  return Blake2bHash<kZCashDigestSize>({payload}, personalizer);
}

// static
void ZCashSerializer::PushOutpoint(const ZCashTransaction::Outpoint& outpoint,
                                   BtcLikeSerializerStream& stream) {
  stream.PushBytes(outpoint.txid);
  stream.Push32(outpoint.index);
}

// static
void ZCashSerializer::PushOutput(const ZCashTransaction::TxOutput& output,
                                 BtcLikeSerializerStream& stream) {
  stream.Push64(output.amount);
  stream.PushSizeAndBytes(output.script_pubkey);
}

// static
void ZCashSerializer::SerializeTransparentInputs(
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
void ZCashSerializer::SerializeTransparentOutputs(
    const ZCashTransaction& tx,
    BtcLikeSerializerStream& stream) {
  stream.PushCompactSize(tx.transparent_part().outputs.size());
  for (const auto& output : tx.transparent_part().outputs) {
    PushOutput(output, stream);
  }
}

// static
std::array<uint8_t, kBlake2bPersonalizerLength>
ZCashSerializer::GetHashPersonalizer(const ZCashTransaction& tx) {
  std::array<uint8_t, kBlake2bPersonalizerLength> result;
  auto span_writer = base::SpanWriter(base::span(result));
  span_writer.Write(base::byte_span_from_cstring(kTxHashPersonalizerPrefix));
  span_writer.WriteU32LittleEndian(tx.consensus_brach_id());
  DCHECK_EQ(span_writer.remaining(), 0u);
  return result;
}

// static
void ZCashSerializer::SerializeSignature(
    const ZCashTransaction& tx,
    ZCashTransaction::TxInput& input,
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& signature) {
  BtcLikeSerializerStream stream;
  stream.PushCompactSize(signature.size() + 1);
  stream.PushBytes(signature);
  stream.Push8(tx.sighash_type());
  stream.PushSizeAndBytes(pubkey);
  input.script_sig = std::move(stream).Take();
}

// static
// https://zips.z.cash/zip-0244#s-2g-txin-sig-digest
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashTxIn(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashAmounts(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashScriptPubKeys(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashPrevouts(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashSequences(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashOutputs(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashTransparentTxId(
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
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::HashTransparentSignature(
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

// static
bool ZCashSerializer::SignTransparentPart(
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction& tx,
    base::FunctionRef<std::array<uint8_t, kZCashDigestSize>(
        const ZCashTransaction&,
        const ZCashTransaction::TxInput&)> compute_sig_digest) {
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

    auto signature_digest = compute_sig_digest(tx, input);

    auto signature = keyring_service.SignMessageByZCashKeyring(
        account_id, key_id, base::span(signature_digest));

    if (!signature) {
      return false;
    }

    SerializeSignature(tx, input, pubkey.value(), signature.value());
  }

  return true;
}

// Dispatch entry points.

// static
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::CalculateTxIdDigest(
    const ZCashTransaction& zcash_transaction) {
  return zcash_transaction.v6_part()
             ? ZCashV6Serializer::CalculateTxIdDigest(zcash_transaction)
             : ZCashV5Serializer::CalculateTxIdDigest(zcash_transaction);
}

// static
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::CalculateSignatureDigest(
    const ZCashTransaction& zcash_transaction,
    const std::optional<ZCashTransaction::TxInput>& input) {
  return zcash_transaction.v6_part()
             ? ZCashV6Serializer::CalculateSignatureDigest(zcash_transaction,
                                                           input)
             : ZCashV5Serializer::CalculateSignatureDigest(zcash_transaction,
                                                           input);
}

// static
std::vector<uint8_t> ZCashSerializer::SerializeRawTransaction(
    const ZCashTransaction& zcash_transaction) {
  return zcash_transaction.v6_part()
             ? ZCashV6Serializer::SerializeRawTransaction(zcash_transaction)
             : ZCashV5Serializer::SerializeRawTransaction(zcash_transaction);
}

// static
bool ZCashSerializer::SignTransparentPart(KeyringService& keyring_service,
                                          const mojom::AccountIdPtr& account_id,
                                          ZCashTransaction& tx) {
  return tx.v6_part()
             ? ZCashV6Serializer::SignTransparentPart(keyring_service,
                                                      account_id, tx)
             : ZCashV5Serializer::SignTransparentPart(keyring_service,
                                                      account_id, tx);
}

}  // namespace brave_wallet
