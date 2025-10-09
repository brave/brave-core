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

// https://adastat.net/transactions/1e89c2a5449d281cbdaed23af716bea50e31fe6b314814e48f1c57c919b2c072
constexpr auto* kTaggedTx =
    "84A400D901028182582073263792876C2BACFBDF46EC13ADEAFD59830954AA84B36A4286"
    "36F075FFE0B101018282583901DA72B7C0C0324CD841D29F68D67D1C10A67AAFF0B59D61"
    "3F2A4BD2336C162A7B0DE7540E0EFEC893D6C92DE70A2E7C9477C098F21A84BFE21B0000"
    "0001A13B8600825839012CE75A18452B9A4C4481A9ECAFE4166CD769ABB3313B03097FC3"
    "98D00E3EF28F8254481D403EB5D26F118D7BA0C09EAF36F0FD1A0AC624A81B000000214A"
    "25425C021A00029309031A0A06305FA100D90102818258206F3253B5847F0DCD7ACF72D0"
    "318429D1365F441F1038002FA44180D45ED5FFDD5840FA67EA8E4968F2552084958442D1"
    "F868D27771DBA4A5858B9AF25D597AF5F5351611991F2F4F3DFF2964E831A184B8AC7206"
    "649C6212C520F827179B2D355D0AF5F6";

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

  // Validate raw bytes
  EXPECT_EQ(restored_tx.raw_tx_bytes, tx_bytes);

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

TEST(CardanoTxDecoderTest, DecodeTransaction_ValidTransactionWithTag) {
  std::vector<uint8_t> tx_bytes;
  ASSERT_TRUE(base::HexStringToBytes(kTaggedTx, &tx_bytes));

  auto decode_result = CardanoTxDecoder::DecodeTransaction(tx_bytes);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result.value();

  // Validate raw bytes
  EXPECT_EQ(restored_tx.raw_tx_bytes, tx_bytes);
  EXPECT_EQ(base::HexEncode(restored_tx.tx_body.raw_body_bytes),
            "A400D901028182582073263792876C2BACFBDF46EC13ADEAFD59830954AA84B36A"
            "428636F075FFE0B101018282583901DA72B7C0C0324CD841D29F68D67D1C10A67A"
            "AFF0B59D613F2A4BD2336C162A7B0DE7540E0EFEC893D6C92DE70A2E7C9477C098"
            "F21A84BFE21B00000001A13B8600825839012CE75A18452B9A4C4481A9ECAFE416"
            "6CD769ABB3313B03097FC398D00E3EF28F8254481D403EB5D26F118D7BA0C09EAF"
            "36F0FD1A0AC624A81B000000214A25425C021A00029309031A0A06305F");

  // Validate inputs
  EXPECT_EQ(restored_tx.tx_body.inputs.size(), 1u);
  EXPECT_EQ(base::HexEncode(restored_tx.tx_body.inputs[0].tx_hash),
            "73263792876C2BACFBDF46EC13ADEAFD59830954AA84B36A428636F075FFE0B1");
  EXPECT_EQ(restored_tx.tx_body.inputs[0].index, 1u);
  EXPECT_FALSE(restored_tx.tx_body.inputs[0].address);
  EXPECT_FALSE(restored_tx.tx_body.inputs[0].amount);

  // Validate outputs
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 2u);
  EXPECT_EQ(restored_tx.tx_body.outputs[0].address.ToString(),
            "addr1q8d89d7qcqeyekzp620k34narsg2v7407z6e6cfl9f9ayvmvzc48kr082s8qa"
            "lkgj0tvjt08pgh8e9rhczv0yx5yhl3qj04knp");
  EXPECT_EQ(restored_tx.tx_body.outputs[0].amount, 7000000000u);
  EXPECT_EQ(restored_tx.tx_body.outputs[1].address.ToString(),
            "addr1qykwwkscg54e5nzysx57etlyzekdw6dtkvcnkqcf0lpe35qw8meglqj5fqw5q"
            "0446fh3rrtm5rqfatek7r735zkxyj5qt7ypw2");
  EXPECT_EQ(restored_tx.tx_body.outputs[1].amount, 142977876572u);
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

TEST(CardanoTxDecoderTest, AddWitnessesToTransaction_ValidSignaturesWithTag) {
  std::vector<CardanoTxDecoder::CardanoSignMessageResult> sign_results;

  // Create first signature result
  CardanoTxDecoder::CardanoSignMessageResult sign_result1;
  sign_result1.public_key = std::vector<uint8_t>(32, 1);
  sign_result1.signature_bytes = std::vector<uint8_t>(64, 2);
  sign_results.push_back(std::move(sign_result1));

  // Create second signature result
  CardanoTxDecoder::CardanoSignMessageResult sign_result2;
  sign_result2.public_key = std::vector<uint8_t>(32, 3);
  sign_result2.signature_bytes = std::vector<uint8_t>(64, 4);
  sign_results.push_back(std::move(sign_result2));

  std::vector<uint8_t> tx_bytes;
  ASSERT_TRUE(base::HexStringToBytes(kTaggedTx, &tx_bytes));

  auto result =
      CardanoTxDecoder::AddWitnessesToTransaction(tx_bytes, sign_results);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(
      base::HexEncode(result.value()),
      "84A400D901028182582073263792876C2BACFBDF46EC13ADEAFD59830954AA84B36A4286"
      "36F075FFE0B101018282583901DA72B7C0C0324CD841D29F68D67D1C10A67AAFF0B59D61"
      "3F2A4BD2336C162A7B0DE7540E0EFEC893D6C92DE70A2E7C9477C098F21A84BFE21B0000"
      "0001A13B8600825839012CE75A18452B9A4C4481A9ECAFE4166CD769ABB3313B03097FC3"
      "98D00E3EF28F8254481D403EB5D26F118D7BA0C09EAF36F0FD1A0AC624A81B000000214A"
      "25425C021A00029309031A0A06305FA100D90102838258206F3253B5847F0DCD7ACF72D0"
      "318429D1365F441F1038002FA44180D45ED5FFDD5840FA67EA8E4968F2552084958442D1"
      "F868D27771DBA4A5858B9AF25D597AF5F5351611991F2F4F3DFF2964E831A184B8AC7206"
      "649C6212C520F827179B2D355D0A82582001010101010101010101010101010101010101"
      "010101010101010101010101015840020202020202020202020202020202020202020202"
      "020202020202020202020202020202020202020202020202020202020202020202020202"
      "020202020202028258200303030303030303030303030303030303030303030303030303"
      "030303030303584004040404040404040404040404040404040404040404040404040404"
      "040404040404040404040404040404040404040404040404040404040404040404040404"
      "F5F6");
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
      0x58, 0x1d,  // Byte array with 29-byte length
      0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, // 28-byte address (all zeros)
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
      0x58, 0x1d,  // Byte array with 29-byte length
      0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
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

  EXPECT_EQ(restored_tx.tx_body.outputs[0].address.ToString(),
            "addr_test1vqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqd9tg5t");
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
      0x58, 0x1d,  // Byte array with 29-byte length
      0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,  // 28-byte address (all zeros)
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x82,        // Second output array with 2 elements [address, amount]
      0x58, 0x1d,  // Byte array with 29-byte length
      0x60, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
      0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
      0x22, 0x22, 0x22, 0x22, 0x22, // 28-byte address (all 0x22)
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

  EXPECT_EQ(restored_tx.tx_body.outputs[0].address.ToString(),
            "addr_test1vqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqd9tg5t");
  EXPECT_EQ(restored_tx.tx_body.outputs[1].address.ToString(),
            "addr_test1vq3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zygswahgq5");
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
