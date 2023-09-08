/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"

#include <utility>

#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

// We use version 2 per
// https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki#specification
constexpr uint32_t kTransactionsVersion = 2;

namespace {

void PushOutpoint(const BitcoinTransaction::Outpoint& outpoint,
                  BitcoinSerializerStream& stream) {
  stream.PushBytesReversed(outpoint.txid);
  stream.Push32AsLE(outpoint.index);
}

void PushScriptCodeForSigninig(const DecodedBitcoinAddress& decoded_address,
                               BitcoinSerializerStream& stream) {
  // TODO(apaymyshev): support more.
  DCHECK_EQ(decoded_address.address_type,
            BitcoinAddressType::kWitnessV0PubkeyHash);
  // See step 5 of
  // https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki#specification
  constexpr uint8_t kPrefix[] = {0x19, 0x76, 0xa9, 0x14};
  constexpr uint8_t kSuffix[] = {0x88, 0xac};
  stream.PushBytes(kPrefix);
  stream.PushBytes(decoded_address.pubkey_hash);
  stream.PushBytes(kSuffix);
}

SHA256HashArray HashPrevouts(const BitcoinTransaction& tx) {
  DCHECK_EQ(tx.sighash_type(), kBitcoinSigHashAll);

  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  for (const auto& input : tx.inputs()) {
    PushOutpoint(input.utxo_outpoint, stream);
  }

  return DoubleSHA256Hash(data);
}

SHA256HashArray HashSequence(const BitcoinTransaction& tx) {
  DCHECK_EQ(tx.sighash_type(), kBitcoinSigHashAll);

  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  for (const auto& input : tx.inputs()) {
    stream.Push32AsLE(input.n_sequence());
  }

  return DoubleSHA256Hash(data);
}

std::vector<uint8_t> AddressToScriptPubkey(const std::string& address) {
  auto decoded_address = DecodeBitcoinAddress(address);
  // TODO(apaymyshev): support more types
  if (decoded_address && decoded_address->address_type ==
                             BitcoinAddressType::kWitnessV0PubkeyHash) {
    auto pubkey_hash = std::move(decoded_address->pubkey_hash);
    CHECK_EQ(pubkey_hash.size(), 20u);

    // https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki#p2wpkh
    std::vector<uint8_t> data;
    BitcoinSerializerStream stream(data);
    stream.Push8AsLE(0);                   // OP_0
    stream.Push8AsLE(pubkey_hash.size());  // OP_14
    stream.PushBytes(pubkey_hash);
    return data;
  }

  NOTREACHED_NORETURN();
}

void PushOutput(const BitcoinTransaction::TxOutput& output,
                BitcoinSerializerStream& stream) {
  stream.Push64AsLE(output.amount);
  stream.PushSizeAndBytes(AddressToScriptPubkey(output.address));
}

SHA256HashArray HashOutputs(const BitcoinTransaction& tx) {
  DCHECK_EQ(tx.sighash_type(), kBitcoinSigHashAll);

  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }

  return DoubleSHA256Hash(data);
}

void PushInputs(const BitcoinTransaction& tx, BitcoinSerializerStream& stream) {
  stream.PushVarInt(tx.inputs().size());
  for (const auto& input : tx.inputs()) {
    PushOutpoint(input.utxo_outpoint, stream);
    // TODO(apaymsyhev): we support only segwit inputs by now, so script_sig
    // should be emtpy.
    DCHECK(input.script_sig.empty());
    stream.PushSizeAndBytes(input.script_sig);
    stream.Push32AsLE(input.n_sequence());
  }
}

void PushOutputs(const BitcoinTransaction& tx,
                 BitcoinSerializerStream& stream) {
  stream.PushVarInt(tx.outputs().size());
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }
}

void PushWitnesses(const BitcoinTransaction& tx,
                   BitcoinSerializerStream& stream) {
  for (const auto& input : tx.inputs()) {
    DCHECK(!input.witness.empty());
    stream.PushBytes(input.witness);
  }
}

}  // namespace

void BitcoinSerializerStream::Push8AsLE(uint8_t i) {
  to().push_back(i);
}

void BitcoinSerializerStream::Push16AsLE(uint16_t i) {
  i = base::ByteSwapToLE16(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  PushBytes(data_to_insert);
}

void BitcoinSerializerStream::Push32AsLE(uint32_t i) {
  i = base::ByteSwapToLE32(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  PushBytes(data_to_insert);
}

void BitcoinSerializerStream::Push64AsLE(uint64_t i) {
  i = base::ByteSwapToLE64(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  PushBytes(data_to_insert);
}

// https://developer.bitcoin.org/reference/transactions.html#compactsize-unsigned-integers
void BitcoinSerializerStream::PushVarInt(uint64_t i) {
  if (i < 0xfd) {
    Push8AsLE(i);
  } else if (i <= 0xffff) {
    Push8AsLE(0xfd);
    Push16AsLE(i);
  } else if (i <= 0xffffffff) {
    Push8AsLE(0xfe);
    Push32AsLE(i);
  } else {
    Push8AsLE(0xff);
    Push64AsLE(i);
  }
}

void BitcoinSerializerStream::PushSizeAndBytes(
    base::span<const uint8_t> bytes) {
  PushVarInt(bytes.size());
  PushBytes(bytes);
}

void BitcoinSerializerStream::PushBytes(base::span<const uint8_t> bytes) {
  to().insert(to().end(), bytes.begin(), bytes.end());
}

void BitcoinSerializerStream::PushBytesReversed(
    base::span<const uint8_t> bytes) {
  to().insert(to().end(), bytes.rbegin(), bytes.rend());
}

// static
absl::optional<SHA256HashArray> BitcoinSerializer::SerializeInputForSign(
    const BitcoinTransaction& tx,
    size_t input_index) {
  CHECK_LT(input_index, tx.inputs().size());
  auto& input = tx.inputs()[input_index];
  auto decoded_address = DecodeBitcoinAddress(input.utxo_address);
  if (!decoded_address) {
    return absl::nullopt;
  }

  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  // https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki#specification
  stream.Push32AsLE(2);                // 1.
  stream.PushBytes(HashPrevouts(tx));  // 2.
  stream.PushBytes(HashSequence(tx));  // 3.

  PushOutpoint(input.utxo_outpoint, stream);            // 4
  PushScriptCodeForSigninig(*decoded_address, stream);  // 5.
  stream.Push64AsLE(input.utxo_value);                  // 6.
  stream.Push32AsLE(input.n_sequence());                // 7.

  stream.PushBytes(HashOutputs(tx));     // 8.
  stream.Push32AsLE(tx.locktime());      // 9.
  stream.Push32AsLE(tx.sighash_type());  // 10.  1 byte but serialized as 4 LE.

  return DoubleSHA256Hash(data);
}

// static
std::vector<uint8_t> BitcoinSerializer::SerializeWitness(
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& pubkey) {
  std::vector<uint8_t> result;
  BitcoinSerializerStream witness_stream(result);
  // https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki#transaction-id
  // https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki#p2wpkh
  witness_stream.PushVarInt(2);
  witness_stream.PushSizeAndBytes(signature);
  witness_stream.PushSizeAndBytes(pubkey);
  return result;
}

// static
std::vector<uint8_t> BitcoinSerializer::SerializeSignedTransaction(
    const BitcoinTransaction& tx) {
  DCHECK(tx.IsSigned());

  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);

  // https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki#specification
  stream.Push32AsLE(kTransactionsVersion);  // version
  stream.Push8AsLE(0);                      // marker
  stream.Push8AsLE(1);                      // flag
  PushInputs(tx, stream);
  PushOutputs(tx, stream);
  PushWitnesses(tx, stream);
  stream.Push32AsLE(tx.locktime());

  return data;
}

}  // namespace brave_wallet
