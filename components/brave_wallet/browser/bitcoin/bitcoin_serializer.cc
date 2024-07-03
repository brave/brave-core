/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"

#include <optional>
#include <utility>

#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

// We use version 2 per
// https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki#specification
constexpr uint32_t kTransactionsVersion = 2;
constexpr uint32_t kWitnessScaleFactor = 4;

namespace {

uint32_t WeightUnitsToVBytes(uint32_t wu) {
  // Need ceiling
  // https://github.com/bitcoin/bitcoin/blob/v25.1/src/policy/policy.cpp
  // https://bitcoincore.org/en/segwit_wallet_dev/#transaction-fee-estimation
  return (wu + kWitnessScaleFactor - 1) / kWitnessScaleFactor;
}

const std::vector<uint8_t>& DummySignature() {
  static std::vector<uint8_t> dummy_signature = []() {
    constexpr size_t kRLength = 32;
    constexpr size_t kSLength = 32;
    std::vector<uint8_t> result;
    result.assign(kRLength + kSLength + 7, 0);
    result[0] = 0x30;
    result[1] = kRLength + kSLength + 4;
    result[2] = 0x02;
    result[3] = kRLength;
    result[4] = 0x01;
    result[4 + kRLength] = 0x02;
    result[5 + kRLength] = kSLength;
    result[6 + kRLength] = 0x01;
    result[6 + kRLength + kSLength] = kBitcoinSigHashAll;
    return result;
  }();
  return dummy_signature;
}

const std::vector<uint8_t>& DummyPubkey() {
  static std::vector<uint8_t> dummy_pubkey = []() {
    constexpr size_t kLenght = 33;
    std::vector<uint8_t> result(kLenght, 0);
    return result;
  }();
  return dummy_pubkey;
}

const std::vector<uint8_t>& DummyWitness() {
  static std::vector<uint8_t> dummy_witness = []() {
    return BitcoinSerializer::SerializeWitness(DummySignature(), DummyPubkey());
  }();
  return dummy_witness;
}

void PushOutpoint(const BitcoinTransaction::Outpoint& outpoint,
                  BtcLikeSerializerStream& stream) {
  stream.PushBytesReversed(outpoint.txid);
  stream.Push32AsLE(outpoint.index);
}

void PushScriptCodeForSigninig(const DecodedBitcoinAddress& decoded_address,
                               BtcLikeSerializerStream& stream) {
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
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    PushOutpoint(input.utxo_outpoint, stream);
  }

  return DoubleSHA256Hash(data);
}

SHA256HashArray HashSequence(const BitcoinTransaction& tx) {
  DCHECK_EQ(tx.sighash_type(), kBitcoinSigHashAll);

  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& input : tx.inputs()) {
    stream.Push32AsLE(input.n_sequence());
  }

  return DoubleSHA256Hash(data);
}

void PushOutput(const BitcoinTransaction::TxOutput& output,
                BtcLikeSerializerStream& stream) {
  stream.Push64AsLE(output.amount);
  CHECK(output.script_pubkey.size());
  stream.PushSizeAndBytes(output.script_pubkey);
}

SHA256HashArray HashOutputs(const BitcoinTransaction& tx) {
  DCHECK_EQ(tx.sighash_type(), kBitcoinSigHashAll);

  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }

  return DoubleSHA256Hash(data);
}

void SerializeInputs(const BitcoinTransaction& tx,
                     BtcLikeSerializerStream& stream) {
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

uint32_t GetVarIntVBytes(uint64_t i) {
  if (i < 0xfd) {
    return 1;
  } else if (i <= 0xffff) {
    return 1 + 2;
  } else if (i <= 0xffffffff) {
    return 1 + 4;
  } else {
    return 1 + 8;
  }
}

uint32_t GetVarArrayVBytes(const std::vector<uint8_t>& var_array) {
  return GetVarIntVBytes(var_array.size()) + var_array.size();
}

uint32_t GetOutpointVBytes(const BitcoinTransaction::Outpoint& outpoint) {
  uint32_t bytes = 0;
  bytes += 32;  // txid
  bytes += 4;   // index
  return bytes;
}

uint32_t GetInputVBytes(const BitcoinTransaction::TxInput& input) {
  uint32_t bytes = 0;
  bytes += GetOutpointVBytes(input.utxo_outpoint);
  DCHECK(input.script_sig.empty());
  bytes += GetVarArrayVBytes(input.script_sig);
  bytes += 4;  // n_sequence
  return bytes;
}

uint32_t GetInputsVBytes(const BitcoinTransaction& tx) {
  uint32_t bytes = 0;
  bytes += GetVarIntVBytes(tx.inputs().size());
  for (const auto& input : tx.inputs()) {
    bytes += GetInputVBytes(input);
  }
  return bytes;
}

uint32_t GetOutputVBytes(const BitcoinTransaction::TxOutput& output) {
  uint32_t bytes = 0;
  bytes += 8;  // amount
  bytes += GetVarArrayVBytes(output.script_pubkey);
  return bytes;
}

uint32_t GetOutputsVBytes(const BitcoinTransaction& tx) {
  uint32_t bytes = 0;
  bytes += GetVarIntVBytes(tx.outputs().size());
  for (const auto& input : tx.outputs()) {
    bytes += GetOutputVBytes(input);
  }
  return bytes;
}

void SerializeOutputs(const BitcoinTransaction& tx,
                      BtcLikeSerializerStream& stream) {
  stream.PushVarInt(tx.outputs().size());
  for (const auto& output : tx.outputs()) {
    PushOutput(output, stream);
  }
}

void SerializeWitnesses(const BitcoinTransaction& tx,
                        BtcLikeSerializerStream& stream) {
  for (const auto& input : tx.inputs()) {
    DCHECK(!input.witness.empty());
    stream.PushBytes(input.witness);
  }
}

uint32_t GetWitnessWeightUnits(const BitcoinTransaction::TxInput& input,
                               bool dummy_signatures) {
  if (dummy_signatures) {
    return DummyWitness().size();
  } else {
    DCHECK(!input.witness.empty());
    return input.witness.size();
  }
}

uint32_t GetWitnessesWeightUnits(const BitcoinTransaction& tx,
                                 bool dummy_signatures) {
  uint32_t weight = 0;
  for (const auto& input : tx.inputs()) {
    weight += GetWitnessWeightUnits(input, dummy_signatures);
  }
  return weight;
}

}  // namespace

std::vector<uint8_t> BitcoinSerializer::AddressToScriptPubkey(
    const std::string& address,
    bool testnet) {
  auto decoded_address = DecodeBitcoinAddress(address);
  if (!decoded_address) {
    return {};
  }

  if (testnet != decoded_address->testnet) {
    return {};
  }

  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);

  // https://github.com/bitcoin/bitcoin/blob/v25.0/src/script/standard.cpp#L302-L325

  // Size is always less than OP_PUSHDATA1, so we encode it as one byte.
  CHECK_LT(decoded_address->pubkey_hash.size(), 0x4cUL);

  if (decoded_address->address_type == BitcoinAddressType::kPubkeyHash) {
    CHECK_EQ(decoded_address->pubkey_hash.size(), 20u);

    stream.Push8AsLE(0x76);                          // OP_DUP
    stream.Push8AsLE(0xa9);                          // OP_HASH
    stream.Push8AsLE(0x14);                          // hash size
    stream.PushBytes(decoded_address->pubkey_hash);  // hash
    stream.Push8AsLE(0x88);                          // OP_EQUALVERIFY
    stream.Push8AsLE(0xac);                          // OP_CHECKSIG

    return data;
  }

  if (decoded_address->address_type == BitcoinAddressType::kScriptHash) {
    CHECK_EQ(decoded_address->pubkey_hash.size(), 20u);

    stream.Push8AsLE(0xa9);                          // OP_HASH
    stream.Push8AsLE(0x14);                          // hash size
    stream.PushBytes(decoded_address->pubkey_hash);  // hash
    stream.Push8AsLE(0x87);                          // OP_EQUAL

    return data;
  }

  if (decoded_address->address_type ==
      BitcoinAddressType::kWitnessV0PubkeyHash) {
    CHECK_EQ(decoded_address->pubkey_hash.size(), 20u);

    stream.Push8AsLE(0x00);                          // OP_0
    stream.Push8AsLE(0x14);                          // hash size
    stream.PushBytes(decoded_address->pubkey_hash);  // hash
    return data;
  }

  if (decoded_address->address_type ==
      BitcoinAddressType::kWitnessV0ScriptHash) {
    CHECK_EQ(decoded_address->pubkey_hash.size(), 32u);

    stream.Push8AsLE(0x00);                          // OP_0
    stream.Push8AsLE(0x20);                          // hash size
    stream.PushBytes(decoded_address->pubkey_hash);  // hash
    return data;
  }

  if (decoded_address->address_type == BitcoinAddressType::kWitnessV1Taproot) {
    CHECK_EQ(decoded_address->pubkey_hash.size(), 32u);

    stream.Push8AsLE(0x51);                          // OP_1
    stream.Push8AsLE(0x20);                          // hash size
    stream.PushBytes(decoded_address->pubkey_hash);  // hash
    return data;
  }

  NOTREACHED_IN_MIGRATION();
  return {};
}

// static
std::optional<SHA256HashArray> BitcoinSerializer::SerializeInputForSign(
    const BitcoinTransaction& tx,
    size_t input_index) {
  CHECK_LT(input_index, tx.inputs().size());
  auto& input = tx.inputs()[input_index];
  auto decoded_address = DecodeBitcoinAddress(input.utxo_address);
  if (!decoded_address) {
    return std::nullopt;
  }
  // TODO(apaymyshev): support other account types.
  if (decoded_address->address_type !=
      BitcoinAddressType::kWitnessV0PubkeyHash) {
    return std::nullopt;
  }

  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
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
  BtcLikeSerializerStream witness_stream(&result);
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
  BtcLikeSerializerStream stream(&data);

  // https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki#specification
  stream.Push32AsLE(kTransactionsVersion);  // version
  stream.Push8AsLE(0);                      // marker
  stream.Push8AsLE(1);                      // flag
  SerializeInputs(tx, stream);
  SerializeOutputs(tx, stream);
  SerializeWitnesses(tx, stream);
  stream.Push32AsLE(tx.locktime());

  return data;
}

// static
uint32_t BitcoinSerializer::CalcOutputVBytesInTransaction(
    const BitcoinTransaction::TxOutput& output) {
  return GetOutputVBytes(output);
}

// static
uint32_t BitcoinSerializer::CalcInputVBytesInTransaction(
    const BitcoinTransaction::TxInput& input) {
  return GetInputVBytes(input) +
         WeightUnitsToVBytes(GetWitnessWeightUnits(input, true));
}

// static
uint32_t BitcoinSerializer::CalcTransactionWeight(const BitcoinTransaction& tx,
                                                  bool dummy_signatures) {
  // TODO(apaymsyhev): we support only segwit inputs by now, so script_sig
  // should be emtpy.
  for (const auto& input : tx.inputs()) {
    DCHECK(input.script_sig.empty());
  }
  const bool is_segwit = true;

  uint32_t weight = 0;

  weight += 4 * kWitnessScaleFactor;  // version
  if (is_segwit) {
    weight += 2;  // marker, flag
  }
  weight += GetInputsVBytes(tx) * kWitnessScaleFactor;
  weight += GetOutputsVBytes(tx) * kWitnessScaleFactor;
  if (is_segwit) {
    weight += GetWitnessesWeightUnits(tx, dummy_signatures);
  }
  weight += 4 * kWitnessScaleFactor;  // lock_time

  return weight;
}

// static
uint32_t BitcoinSerializer::CalcTransactionVBytes(const BitcoinTransaction& tx,
                                                  bool dummy_signatures) {
  return WeightUnitsToVBytes(CalcTransactionWeight(tx, dummy_signatures));
}

}  // namespace brave_wallet
