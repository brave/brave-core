/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

CardanoTransaction GetUnsignedReferenceTransaction() {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input;
  input.utxo_outpoint.txid = test::HexToArray<32>(
      "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925");
  input.utxo_outpoint.index = 0;
  input.utxo_value = 34451133;
  tx.AddInput(std::move(input));

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(
      "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj4z53"
      "l7lh5u7z08l0rvp49ht88s5uskl6tsl");
  output1.amount = 10000000;
  tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(
      "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefvjmpww7"
      "f7w9gwem3x6gcm3ulw3kpcgws9sgrhg");
  output2.amount = 24282816;
  output2.type = CardanoTransaction::TxOutputType::kChange;
  tx.AddOutput(std::move(output2));

  tx.set_invalid_after(149770436);

  return tx;
}

CardanoTransaction GetSignedReferenceTransaction() {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input;
  input.utxo_outpoint.txid = test::HexToArray<32>(
      "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925");
  input.utxo_outpoint.index = 0;
  input.utxo_value = 34451133;
  tx.AddInput(std::move(input));

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(
      "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj4z53"
      "l7lh5u7z08l0rvp49ht88s5uskl6tsl");
  output1.amount = 10000000;
  tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(
      "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefvjmpww7"
      "f7w9gwem3x6gcm3ulw3kpcgws9sgrhg");
  output2.amount = 24282816;
  output2.type = CardanoTransaction::TxOutputType::kChange;
  tx.AddOutput(std::move(output2));

  CardanoTransaction::TxWitness witness;
  witness.witness_bytes = test::HexToArray<96>(
      "e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af"
      "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161"
      "b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");

  tx.SetWitnesses({witness});

  tx.set_invalid_after(149770436);

  return tx;
}

std::vector<uint8_t> CreateInvalidCborData() {
  return {0x00, 0x01, 0x02, 0x03};  // Invalid CBOR
}

}  // namespace

TEST(CardanoTxDecoderTest, DecodeTransaction_ValidTransaction) {
  auto tx = GetSignedReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  auto decode_result = CardanoTxDecoder::DecodeTransaction(tx_bytes);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result.value();

  EXPECT_EQ(restored_tx.raw_bytes, tx_bytes);

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), tx.inputs().size());
  for (size_t i = 0; i < tx.inputs().size(); i++) {
    EXPECT_EQ(restored_tx.tx_body.inputs[i].tx_hash,
              tx.inputs()[i].utxo_outpoint.txid);
    EXPECT_EQ(restored_tx.tx_body.inputs[i].index,
              tx.inputs()[i].utxo_outpoint.index);
  }

  EXPECT_EQ(restored_tx.tx_body.outputs.size(), tx.outputs().size());
  for (size_t i = 0; i < tx.outputs().size(); i++) {
    EXPECT_EQ(restored_tx.tx_body.outputs[i].address, tx.outputs()[i].address);
    EXPECT_EQ(restored_tx.tx_body.outputs[i].amount, tx.outputs()[i].amount);
  }
}

TEST(CardanoTxDecoderTest, DecodeTransaction_InvalidCborData) {
  auto invalid_cbor = CreateInvalidCborData();

  auto decode_result = CardanoTxDecoder::DecodeTransaction(invalid_cbor);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_EmptyData) {
  std::vector<uint8_t> empty_data;

  auto decode_result = CardanoTxDecoder::DecodeTransaction(empty_data);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_MalformedTransaction) {
  std::vector<uint8_t> malformed_cbor = {
      0x82,  // Array with 2 elements
      0x01,  // First element (should be transaction body)
      0x02   // Second element (should be witness set)
             // Missing proper transaction structure
  };

  auto decode_result = CardanoTxDecoder::DecodeTransaction(malformed_cbor);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, AddWitnessesToTransaction_ValidSignatures) {
  auto tx = GetUnsignedReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  std::vector<CardanoTxDecoder::CardanoSignMessageResult> sign_results;

  CardanoTxDecoder::CardanoSignMessageResult sign_result1;
  sign_result1.public_key = std::vector<uint8_t>(32, 1);
  sign_result1.signature_bytes = std::vector<uint8_t>(64, 2);
  sign_results.push_back(std::move(sign_result1));

  CardanoTxDecoder::CardanoSignMessageResult sign_result2;
  sign_result2.public_key = std::vector<uint8_t>(32, 3);
  sign_result2.signature_bytes = std::vector<uint8_t>(64, 4);
  sign_results.push_back(std::move(sign_result2));

  // Create expected signed transaction
  auto tx_with_signatures = GetUnsignedReferenceTransaction();
  std::vector<CardanoTransaction::TxWitness> witnesses;
  for (const auto& sign_result : sign_results) {
    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));

    span_writer.Write(sign_result.public_key);
    span_writer.Write(sign_result.signature_bytes);
    witnesses.push_back(witness);
  }
  tx_with_signatures.SetWitnesses(witnesses);

  auto expected_signed_bytes =
      CardanoTransactionSerializer().SerializeTransaction(tx_with_signatures);

  auto result =
      CardanoTxDecoder::AddWitnessesToTransaction(tx_bytes, sign_results);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(expected_signed_bytes, result.value());
}

TEST(CardanoTxDecoderTest, AddWitnessesToTransaction_EmptySignatures) {
  auto tx = GetUnsignedReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  std::vector<CardanoTxDecoder::CardanoSignMessageResult> empty_sign_results;

  auto result =
      CardanoTxDecoder::AddWitnessesToTransaction(tx_bytes, empty_sign_results);

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(tx_bytes, result.value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_InputIndexOverflow) {
  // clang-format off
  std::vector<uint8_t> cbor_with_large_index = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x82,        // First input array with 2 elements
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      0x1b, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x00,  // Large integer (2^32 + 1)
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x82,        // First output array with 2 elements
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result =
      CardanoTxDecoder::DecodeTransaction(cbor_with_large_index);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_NegativeIntegerValues) {
  // clang-format off
  std::vector<uint8_t> cbor_with_negative_values = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x82,        // First input array with 2 elements
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      0x20,        // Negative index: -1
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x82,        // First output array with 2 elements
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x21,        // Negative amount: -2
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result =
      CardanoTxDecoder::DecodeTransaction(cbor_with_negative_values);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_InvalidIntegerTypes) {
  // clang-format off
  std::vector<uint8_t> cbor_with_invalid_types = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x82,        // First input array with 2 elements
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      0x61, 0x30,  // Text string "0" instead of integer 0
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x82,        // First output array with 2 elements
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x61, 0x31,  // Text string "1" instead of integer 1
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result =
      CardanoTxDecoder::DecodeTransaction(cbor_with_invalid_types);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_InputWithInsufficientElements) {
  // clang-format off
  std::vector<uint8_t> cbor_with_insufficient_input_elements = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x81,        // First input array with only 1 element (should be 2)
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      // Missing index element
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x82,        // First output array with 2 elements
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(
      cbor_with_insufficient_input_elements);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_InputWithExcessElements) {
  // clang-format off
  std::vector<uint8_t> cbor_with_excess_input_elements = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x83,        // First input array with 3 elements (should be 2)
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      0x00,        // Index: 0
      0x00,        // Extra element (should not be present)
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x82,        // First output array with 2 elements
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result =
      CardanoTxDecoder::DecodeTransaction(cbor_with_excess_input_elements);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_OutputWithInsufficientElements) {
  // clang-format off
  std::vector<uint8_t> cbor_with_insufficient_output_elements = {
      0x82,        // Array with 2 elements (transaction body and witness set)
      0xa2,        // Transaction body map with 2 key-value pairs
      0x00,        // Key 0 (inputs)
      0x81,        // Array with 1 input
      0x82,        // First input array with 2 elements
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte transaction hash (all zeros)
      0x00,        // Index: 0
      0x01,        // Key 1 (outputs)
      0x81,        // Array with 1 output
      0x81,        // First output array with only 1 element (should be 2)
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      // Missing amount element
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(
      cbor_with_insufficient_output_elements);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_OutputWithExcessElements) {
  // clang-format off
  std::vector<uint8_t> cbor_with_excess_output = {
      0x82,        // Main array: [transaction_body, witness_set]
      0xa2,        // Transaction body map with 2 entries
      0x00,        // Key 0: inputs
      0x81,        // Array with 1 input
      0x82,        // Input array with 2 elements [tx_hash, index]
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte tx_hash (all zeros)
      0x00,        // Index: 0
      0x01,        // Key 1: outputs
      0x81,        // Array with 1 output
      0x83,        // Output array with 3 elements [address, amount, extra]
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x61, 0x78,  // Extra element: text string "x"
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result =
      CardanoTxDecoder::DecodeTransaction(cbor_with_excess_output);
  EXPECT_TRUE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_ValidMinimalTransaction) {
  // clang-format off
  std::vector<uint8_t> valid_minimal_cbor = {
      0x82,        // Main array: [transaction_body, witness_set]
      0xa2,        // Transaction body map with 2 entries
      0x00,        // Key 0: inputs
      0x81,        // Array with 1 input
      0x82,        // Input array with 2 elements [tx_hash, index]
      0x58, 0x20,  // Byte array with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte tx_hash (all zeros)
      0x00,        // Index: 0
      0x01,        // Key 1: outputs
      0x81,        // Array with 1 output
      0x82,        // Output array with 2 elements [address, amount]
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(valid_minimal_cbor);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result.value();

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), 1u);
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 1u);

  EXPECT_EQ(restored_tx.tx_body.inputs[0].index, 0u);

  std::array<uint8_t, 32> expected_tx_hash = {};
  EXPECT_EQ(restored_tx.tx_body.inputs[0].tx_hash, expected_tx_hash);

  EXPECT_EQ(restored_tx.tx_body.outputs[0].amount, 10000000u);

  std::vector<uint8_t> expected_address(28, 0);
  EXPECT_EQ(restored_tx.tx_body.outputs[0].address.ToCborBytes(),
            expected_address);
}

TEST(CardanoTxDecoderTest,
     DecodeTransaction_ValidTransactionWithMultipleInputsOutputs) {
  // clang-format off
  std::vector<uint8_t> valid_multi_cbor = {
      0x82,        // Main array: [transaction_body, witness_set]
      0xa2,        // Transaction body map with 2 entries
      0x00,        // Key 0: inputs
      0x82,        // Array with 2 inputs
      0x82,        // First input array with 2 elements [tx_hash, index]
      0x58, 0x20,  // Byte string with 32-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 32-byte tx_hash (all zeros)
      0x00,        // Index: 0
      0x82,        // Second input array with 2 elements [tx_hash, index]
      0x58, 0x20,  // Byte array with 32-byte length
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11,  // 32-byte tx_hash (all 0x11)
      0x01,        // Index: 1
      0x01,        // Key 1: outputs
      0x82,        // Array with 2 outputs
      0x82,        // First output array with 2 elements [address, amount]
      0x58, 0x1c,  // Byte array with 28-byte length
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x82,        // Second output array with 2 elements [address, amount]
      0x58, 0x1c,  // Byte array with 28-byte length
      0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
      0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
      0x22, 0x22, 0x22, 0x22,  // 28-byte address (all 0x22)
      0x1a, 0x00, 0x0f, 0x42, 0x40,  // Amount: 1000000
      0x80,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(valid_multi_cbor);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result.value();

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), 2u);
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 2u);

  EXPECT_EQ(restored_tx.tx_body.inputs[0].index, 0u);
  EXPECT_EQ(restored_tx.tx_body.inputs[1].index, 1u);

  std::array<uint8_t, 32> expected_tx_hash_1 = {};
  EXPECT_EQ(restored_tx.tx_body.inputs[0].tx_hash, expected_tx_hash_1);

  std::array<uint8_t, 32> expected_tx_hash_2 = {};
  std::fill(expected_tx_hash_2.begin(), expected_tx_hash_2.end(), 0x11);
  EXPECT_EQ(restored_tx.tx_body.inputs[1].tx_hash, expected_tx_hash_2);

  EXPECT_EQ(restored_tx.tx_body.outputs[0].amount, 10000000u);
  EXPECT_EQ(restored_tx.tx_body.outputs[1].amount, 1000000u);

  std::vector<uint8_t> expected_address_1(28, 0);
  EXPECT_EQ(restored_tx.tx_body.outputs[0].address.ToCborBytes(),
            expected_address_1);

  std::vector<uint8_t> expected_address_2(28, 0x22);
  EXPECT_EQ(restored_tx.tx_body.outputs[1].address.ToCborBytes(),
            expected_address_2);
}

TEST(CardanoTxDecoderTest, AddWitnessesToTransaction_InvalidTransactionData) {
  auto invalid_tx_bytes = CreateInvalidCborData();

  std::vector<CardanoTxDecoder::CardanoSignMessageResult> sign_results;
  CardanoTxDecoder::CardanoSignMessageResult sign_result;
  sign_result.public_key = std::vector<uint8_t>(32, 1);
  sign_result.signature_bytes = std::vector<uint8_t>(64, 2);
  sign_results.push_back(std::move(sign_result));

  auto result = CardanoTxDecoder::AddWitnessesToTransaction(invalid_tx_bytes,
                                                            sign_results);
  EXPECT_FALSE(result.has_value());
}

}  // namespace brave_wallet
