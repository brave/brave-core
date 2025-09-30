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
  // https://adastat.net/transactions/a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba

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
  // https://adastat.net/transactions/a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba

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

// Helper function to create invalid CBOR data for testing
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
  EXPECT_EQ(restored_tx.raw_bytes, tx_bytes);

  // Validate inputs
  EXPECT_EQ(restored_tx.tx_body.inputs.size(), tx.inputs().size());
  for (size_t i = 0; i < tx.inputs().size(); i++) {
    EXPECT_EQ(restored_tx.tx_body.inputs[i].tx_hash,
              tx.inputs()[i].utxo_outpoint.txid);
    EXPECT_EQ(restored_tx.tx_body.inputs[i].index,
              tx.inputs()[i].utxo_outpoint.index);
  }

  // Validate outputs
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
  // Create malformed CBOR that looks like a transaction but has invalid
  // structure
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

  // Should still return a result (transaction with empty witness set)
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(tx_bytes, result.value());
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
