/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_decoder.h"

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
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

constexpr auto* kMockCardanoSignature =
    "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161"
    "b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802";
constexpr auto* kMockCardanoPubkey =
    "e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af";

CardanoTransaction GetSignedReferenceTransaction() {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input(*CardanoAddress::FromString(
      "addr1qxkp8cu47ylplrt2m9atxrex7kh432k95uclun7khnw89ksw8meglqj5fqw5q0446fh"
      "3rrtm5rqfatek7r735zkxyj5qky72dj"));
  input.utxo_outpoint.txid = test::HexToArray<32>(
      "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925");
  input.utxo_outpoint.index = 0;
  input.coin_value.lovelace_amount = 34451133;
  tx.AddInput(std::move(input));

  CardanoTransaction::TxOutput output1(*CardanoAddress::FromString(
      "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj4z53"
      "l7lh5u7z08l0rvp49ht88s5uskl6tsl"));
  output1.coin_value.lovelace_amount = 10000000;
  tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2(*CardanoAddress::FromString(
      "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefvjmpww7"
      "f7w9gwem3x6gcm3ulw3kpcgws9sgrhg"));
  output2.coin_value.lovelace_amount = 24282816;
  output2.type = CardanoTransaction::TxOutputType::kChange;
  tx.AddOutput(std::move(output2));

  CardanoTransaction::TxWitness witness;
  witness.public_key = test::HexToArray<32>(kMockCardanoPubkey);
  witness.signature = test::HexToArray<64>(kMockCardanoSignature);

  tx.SetWitnesses({witness});

  tx.set_invalid_after(149770436);

  return tx;
}

std::vector<uint8_t> CreateInvalidCborData() {
  return {0x00, 0x01, 0x02, 0x03};  // Invalid CBOR
}

}  // namespace

TEST(CardanoTxDecoderTest, EncodeTransaction_ValidTransaction) {
  auto tx = GetSignedReferenceTransaction();

  EXPECT_EQ(base::HexEncodeLower(
                *CardanoTxDecoder::EncodeTransaction(*tx.ToSerializableTx())),
            "84a400d9010281825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732c"
            "c6240add8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff"
            "61d29d9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde360"
            "6a5bace785391a0098968082583901e057e6ff439d606a3e6c47a00b8677340984"
            "61b83ad9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79"
            "f746c1c21d1a017286c00200031a08ed50c4a100d9010281825820e68ca4655409"
            "8776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fd"
            "c64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca43"
            "9b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");
}

TEST(CardanoTxDecoderTest, EncodeTransaction_FailsOnDuplicateInputs) {
  auto tx = GetSignedReferenceTransaction();
  // Duplicate input with the same outpoint. Encoding should fail.
  tx.AddInput(tx.inputs()[0]);

  EXPECT_FALSE(CardanoTxDecoder::EncodeTransaction(*tx.ToSerializableTx()));

  // Give that new inputs a different outpoint index. Encoding should succeed.
  // (Tx is still invalid as amounts don't match).
  tx.inputs_[1].utxo_outpoint.index = 10;

  EXPECT_EQ(base::HexEncodeLower(
                *CardanoTxDecoder::EncodeTransaction(*tx.ToSerializableTx())),
            "84a400d9010282825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732c"
            "c6240add8c71edb92500825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5"
            "eb732cc6240add8c71edb9250a01828258390144e5e8699ab31de351be61dfeb7c"
            "220eff61d29d9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3"
            "fde3606a5bace785391a0098968082583901e057e6ff439d606a3e6c47a00b8677"
            "34098461b83ad9943242b6bc04b7b276465449b932964b6173bc9f38a876771369"
            "18dc79f746c1c21d1a017286c00200031a08ed50c4a100d9010281825820e68ca4"
            "6554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd8"
            "3381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b360"
            "9bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");
}

TEST(CardanoTxDecoderTest, DecodeTransaction_ValidTransaction) {
  auto tx = GetSignedReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  auto decode_result = CardanoTxDecoder::DecodeTransaction(*tx_bytes);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result.value().tx;

  // Validate raw bytes
  EXPECT_EQ(decode_result->raw_tx_bytes, tx_bytes);

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), tx.inputs().size());
  for (size_t i = 0; i < tx.inputs().size(); i++) {
    EXPECT_EQ(restored_tx.tx_body.inputs[i].tx_hash,
              tx.inputs()[i].utxo_outpoint.txid);
    EXPECT_EQ(restored_tx.tx_body.inputs[i].index,
              tx.inputs()[i].utxo_outpoint.index);
  }

  EXPECT_EQ(restored_tx.tx_body.outputs.size(), tx.outputs().size());
  for (size_t i = 0; i < tx.outputs().size(); i++) {
    EXPECT_EQ(restored_tx.tx_body.outputs[i].address_bytes,
              tx.outputs()[i].address.ToCborBytes());
    EXPECT_EQ(restored_tx.tx_body.outputs[i].coin_value.lovelace_amount,
              tx.outputs()[i].coin_value.lovelace_amount);
  }
}

TEST(CardanoTxDecoderTest, DecodeTransaction_MaxSizeLimit) {
  auto tx = GetSignedReferenceTransaction();
  for (auto i = 0; i < 1000; ++i) {
    auto input = tx.inputs()[0];
    input.utxo_outpoint.index = i + 100;
    tx.AddInput(input);
    auto tx_bytes = *CardanoTransactionSerializer().SerializeTransaction(tx);
    if (tx_bytes.size() > 16 * 1024) {
      break;
    }
    auto decode_result = CardanoTxDecoder::DecodeTransaction(tx_bytes);
    EXPECT_TRUE(decode_result.has_value());
  }

  auto tx_bytes = *CardanoTransactionSerializer().SerializeTransaction(tx);
  EXPECT_GT(tx_bytes.size(), 16u * 1024u);
  auto decode_result = CardanoTxDecoder::DecodeTransaction(tx_bytes);
  EXPECT_FALSE(decode_result.has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_ValidTransactionWithTag) {
  std::vector<uint8_t> tx_bytes;
  ASSERT_TRUE(base::HexStringToBytes(kTaggedTx, &tx_bytes));

  auto decode_result = CardanoTxDecoder::DecodeTransaction(tx_bytes);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result->tx;

  // Validate raw bytes
  EXPECT_EQ(decode_result->raw_tx_bytes, tx_bytes);
  EXPECT_EQ(base::HexEncode(decode_result->raw_body_bytes),
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

  // Validate outputs
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 2u);
  EXPECT_EQ(CardanoAddress::FromCborBytes(
                restored_tx.tx_body.outputs[0].address_bytes)
                ->ToString(),
            "addr1q8d89d7qcqeyekzp620k34narsg2v7407z6e6cfl9f9ayvmvzc48kr082s8qa"
            "lkgj0tvjt08pgh8e9rhczv0yx5yhl3qj04knp");
  EXPECT_EQ(restored_tx.tx_body.outputs[0].coin_value.lovelace_amount,
            7000000000u);
  EXPECT_EQ(CardanoAddress::FromCborBytes(
                restored_tx.tx_body.outputs[1].address_bytes)
                ->ToString(),
            "addr1qykwwkscg54e5nzysx57etlyzekdw6dtkvcnkqcf0lpe35qw8meglqj5fqw5q"
            "0446fh3rrtm5rqfatek7r735zkxyj5qt7ypw2");
  EXPECT_EQ(restored_tx.tx_body.outputs[1].coin_value.lovelace_amount,
            142977876572u);
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

TEST(CardanoTxDecoderTest, DecodeTransaction_EmptyTxArray) {
  std::vector<uint8_t> tx_cbor = {0x80};  // empty cbor array
  EXPECT_FALSE(CardanoTxDecoder::DecodeTransaction(tx_cbor).has_value());
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
      0xa3,        // Transaction body map with 3 entries
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
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,  // Fee: 10000
      0xa0,        // Empty witness set
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
      0xa3,        // Transaction body map with 3 entries
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
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,  // Fee: 10000
      0xa0,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(valid_minimal_cbor);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result->tx;

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), 1u);
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 1u);

  EXPECT_EQ(restored_tx.tx_body.inputs[0].index, 0u);

  std::array<uint8_t, 32> expected_tx_hash = {};
  EXPECT_EQ(restored_tx.tx_body.inputs[0].tx_hash, expected_tx_hash);

  EXPECT_EQ(restored_tx.tx_body.outputs[0].coin_value.lovelace_amount,
            10000000u);

  std::vector<uint8_t> expected_address(28, 0);
  EXPECT_EQ(restored_tx.tx_body.outputs[0].address_bytes, expected_address);

  EXPECT_TRUE(restored_tx.tx_body.withdrawals.empty());
  EXPECT_TRUE(restored_tx.tx_body.mint.empty());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_WithWithdrawals) {
  // Minimal tx body with key 5: withdrawals = { reward_account => coin }.
  // clang-format off
  std::vector<uint8_t> cbor = {
      0x82,        // [transaction_body, witness_set]
      0xa4,        // body map with 4 entries
      0x00,        // Key 0: inputs
      0x81,        // Array with 1 input
      0x82, 0x58, 0x20,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,        // Index: 0
      0x01,        // Key 1: outputs
      0x81,
      0x82, 0x58, 0x1c,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,  // Fee: 10000
      0x05,        // Key 5: withdrawals
      0xa1,        // Map with 1 entry
      0x58, 0x1d,  // reward_account: 29 bytes
      0xe0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11,
      0x1a, 0x00, 0x0f, 0x42, 0x40,  // coin: 1000000
      0xa0,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(cbor);
  ASSERT_TRUE(decode_result.has_value());

  const auto& withdrawals = decode_result->tx.tx_body.withdrawals;
  ASSERT_EQ(withdrawals.size(), 1u);
  EXPECT_EQ(withdrawals[0].coin, 1000000u);

  std::vector<uint8_t> expected_reward_account = {
      0xe0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
  EXPECT_EQ(withdrawals[0].reward_account, expected_reward_account);
  EXPECT_TRUE(decode_result->tx.tx_body.mint.empty());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_WithMint) {
  // Minimal tx body with key 9: mint (positive and negative amounts).
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  ASSERT_EQ(foo_token.size(), 31u);  // 28 policy + 3 name
  ASSERT_EQ(bar_token.size(), 31u);

  // clang-format off
  std::vector<uint8_t> cbor = {
      0x82,        // [transaction_body, witness_set]
      0xa4,        // body map with 4 entries
      0x00,        // Key 0: inputs
      0x81,
      0x82, 0x58, 0x20,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,
      0x01,        // Key 1: outputs
      0x81,
      0x82, 0x58, 0x1c,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x1a, 0x00, 0x98, 0x96, 0x80,
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,
      0x09,        // Key 9: mint
      0xa2,        // Map with 2 policies
      // policy for "foo" (28 * 0x66)
      0x58, 0x1c,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66,
      0xa1,        // asset map
      0x43, 0x66, 0x6f, 0x6f,  // "foo"
      0x18, 0x64,              // +100
      // policy for "bar" (28 * 0x62)
      0x58, 0x1c,
      0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
      0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
      0x62, 0x62, 0x62, 0x62,
      0xa1,
      0x43, 0x62, 0x61, 0x72,  // "bar"
      0x38, 0x63,              // -100
      0xa0,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(cbor);
  ASSERT_TRUE(decode_result.has_value());

  const auto& mint = decode_result->tx.tx_body.mint;
  ASSERT_EQ(mint.size(), 2u);
  EXPECT_TRUE(decode_result->tx.tx_body.withdrawals.empty());

  // Policies are unordered in CBOR map; match by token_id.
  auto find_amount = [&](const std::vector<uint8_t>& token_id) -> int64_t {
    for (const auto& entry : mint) {
      if (entry.token_id == token_id) {
        return entry.amount;
      }
    }
    ADD_FAILURE() << "mint token not found";
    return 0;
  };
  EXPECT_EQ(find_amount(foo_token), 100);
  EXPECT_EQ(find_amount(bar_token), -100);
}

TEST(CardanoTxDecoderTest, DecodeTransaction_MintZeroAmountRejected) {
  // clang-format off
  std::vector<uint8_t> cbor = {
      0x82,
      0xa4,
      0x00,
      0x81,
      0x82, 0x58, 0x20,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,
      0x01,
      0x81,
      0x82, 0x58, 0x1c,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x1a, 0x00, 0x98, 0x96, 0x80,
      0x02,
      0x1a, 0x00, 0x00, 0x27, 0x10,
      0x09,        // Key 9: mint
      0xa1,
      0x58, 0x1c,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x66, 0x66, 0x66,
      0xa1,
      0x43, 0x66, 0x6f, 0x6f,  // "foo"
      0x00,                   // amount 0 — invalid nonzero_int64
      0xa0,
  };
  // clang-format on

  EXPECT_FALSE(CardanoTxDecoder::DecodeTransaction(cbor).has_value());
}

TEST(CardanoTxDecoderTest, DecodeTransaction_WithCertificatesRejected) {
  // Certificates (key 4) are not supported, so any transaction that carries
  // them must be rejected.
  // clang-format off
  std::vector<uint8_t> cbor = {
      0x82,        // [transaction_body, witness_set]
      0xa4,        // body map with 4 entries
      0x00,        // Key 0: inputs
      0x81,        // Array with 1 input
      0x82, 0x58, 0x20,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,        // Index: 0
      0x01,        // Key 1: outputs
      0x81,
      0x82, 0x58, 0x1c,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x1a, 0x00, 0x98, 0x96, 0x80,  // Amount: 10000000
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,  // Fee: 10000
      0x04,        // Key 4: certificates
      0x81,        // Array with 1 certificate
      0x82,        // certificate array [type, stake_credential]
      0x00,        // stake_registration (type 0)
      0x82,        // credential array [type, addr_keyhash]
      0x00,        // key hash credential (type 0)
      0x58, 0x1c,  // addr_keyhash: 28 bytes
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11,
      0xa0,        // Empty witness set
  };
  // clang-format on

  EXPECT_FALSE(CardanoTxDecoder::DecodeTransaction(cbor).has_value());
}

TEST(CardanoTxDecoderTest,
     DecodeTransaction_ValidTransactionWithMultipleInputsOutputs) {
  // clang-format off
  std::vector<uint8_t> valid_multi_cbor = {
      0x82,        // Main array: [transaction_body, witness_set]
      0xa3,        // Transaction body map with 2 entries
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
      0x02,        // Key 2: fee
      0x1a, 0x00, 0x00, 0x27, 0x10,  // Fee: 10000
      0xa0,        // Empty witness set
  };
  // clang-format on

  auto decode_result = CardanoTxDecoder::DecodeTransaction(valid_multi_cbor);
  EXPECT_TRUE(decode_result.has_value());

  const auto& restored_tx = decode_result->tx;

  EXPECT_EQ(restored_tx.tx_body.inputs.size(), 2u);
  EXPECT_EQ(restored_tx.tx_body.outputs.size(), 2u);

  EXPECT_EQ(restored_tx.tx_body.inputs[0].index, 0u);
  EXPECT_EQ(restored_tx.tx_body.inputs[1].index, 1u);

  std::array<uint8_t, 32> expected_tx_hash_1 = {};
  EXPECT_EQ(restored_tx.tx_body.inputs[0].tx_hash, expected_tx_hash_1);

  std::array<uint8_t, 32> expected_tx_hash_2 = {};
  std::fill(expected_tx_hash_2.begin(), expected_tx_hash_2.end(), 0x11);
  EXPECT_EQ(restored_tx.tx_body.inputs[1].tx_hash, expected_tx_hash_2);

  EXPECT_EQ(restored_tx.tx_body.outputs[0].coin_value.lovelace_amount,
            10000000u);
  EXPECT_EQ(restored_tx.tx_body.outputs[1].coin_value.lovelace_amount,
            1000000u);

  std::vector<uint8_t> expected_address_1(28, 0);
  EXPECT_EQ(restored_tx.tx_body.outputs[0].address_bytes, expected_address_1);

  std::vector<uint8_t> expected_address_2(28, 0x22);
  EXPECT_EQ(restored_tx.tx_body.outputs[1].address_bytes, expected_address_2);
}

TEST(CardanoTxDecoderTest, EncodeDecodeCoinValue) {
  EXPECT_FALSE(CardanoTxDecoder::DecodeCoinValue({}));

  struct TestCase {
    cardano_rpc::CoinValue coin_value;
    std::string expected_hex;
  };

  std::vector<TestCase> test_cases = {
      {cardano_rpc::CoinValue(1000000u, {}), "1a000f4240"},
      {cardano_rpc::CoinValue(UINT64_MAX, {}), "1bffffffffffffffff"},
      {cardano_rpc::CoinValue(UINT64_MAX / 2, {}), "1b7fffffffffffffff"},
      {cardano_rpc::CoinValue(1000000u, {{GetMockTokenId("foo"), 1000000u}}),
       "821a000f4240a1581c66666666666666666666666666666666666666666666666666666"
       "666a143666f6f1a000f4240"},
      {cardano_rpc::CoinValue(1000000u, {{GetMockTokenId("foo"), 1000000u},
                                         {GetMockTokenId("bar"), 123u}}),
       "821a000f4240a2581c62626262626262626262626262626262626262626262626262626"
       "262a143626172187b581c66666666666666666666666666666666666666666666666666"
       "666666a143666f6f1a000f4240"},
      {cardano_rpc::CoinValue(1000000u, {{GetMockTokenId("foo"), 1000000u},
                                         {GetMockTokenId("bar"), 123u},
                                         {GetMockTokenId("baz"), UINT64_MAX}}),
       "821a000f4240a2581c62626262626262626262626262626262626262626262626262626"
       "262a243626172187b4362617a1bffffffffffffffff581c666666666666666666666666"
       "66666666666666666666666666666666a143666f6f1a000f4240"},

  };

  for (auto test : test_cases) {
    SCOPED_TRACE(test.expected_hex);
    auto serialized = *CardanoTxDecoder::EncodeCoinValue(test.coin_value);
    EXPECT_EQ(base::HexEncodeLower(serialized), test.expected_hex);
    EXPECT_EQ(test.coin_value,
              CardanoTxDecoder::DecodeCoinValue(serialized).value());
  }
}

TEST(CardanoTxDecoderTest, EncodeTransactionOutput) {
  CardanoTxDecoder::SerializableTxOutput output(
      CardanoAddress::FromString(kMockCardanoAddress1)->ToCborBytes(),
      cardano_rpc::CoinValue(1000000u, {}));

  EXPECT_EQ(base::HexEncodeLower(
                CardanoTxDecoder::EncodeTransactionOutput(output).value()),
            "8258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb"
            "20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace785391a000f4240");

  output.coin_value.tokens.emplace(GetMockTokenId("foo"), 1000000u);
  EXPECT_EQ(
      base::HexEncodeLower(
          CardanoTxDecoder::EncodeTransactionOutput(output).value()),
      "8258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb20324c"
      "1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240a1581c666666"
      "66666666666666666666666666666666666666666666666666a143666f6f1a000f4240");

  output.coin_value.tokens.emplace(GetMockTokenId("bar"), 123u);
  EXPECT_EQ(base::HexEncodeLower(
                CardanoTxDecoder::EncodeTransactionOutput(output).value()),
            "8258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb"
            "20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240"
            "a2581c62626262626262626262626262626262626262626262626262626262a143"
            "626172187b581c6666666666666666666666666666666666666666666666666666"
            "6666a143666f6f1a000f4240");

  output.coin_value.tokens.emplace(GetMockTokenId("baz"), UINT64_MAX);
  EXPECT_EQ(base::HexEncodeLower(
                CardanoTxDecoder::EncodeTransactionOutput(output).value()),
            "8258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb"
            "20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240"
            "a2581c62626262626262626262626262626262626262626262626262626262a243"
            "626172187b4362617a1bffffffffffffffff581c66666666666666666666666666"
            "666666666666666666666666666666a143666f6f1a000f4240");
}

TEST(CardanoTxDecoderTest, EncodeUtxo) {
  CardanoTxDecoder::SerializableTxInput input(
      test::HexToArray<32>(kMockCardanoTxid), 7);

  CardanoTxDecoder::SerializableTxOutput output(
      CardanoAddress::FromString(kMockCardanoAddress1)->ToCborBytes(),
      cardano_rpc::CoinValue(1000000u, {}));

  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeUtxo(input, output).value()),
      "828258207e2aeed860faf61b0513e9807be633a90e3260480ebc46b53ea99c497195fc29"
      "078258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb2032"
      "4c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace785391a000f4240");

  output.coin_value.tokens.emplace(GetMockTokenId("foo"), 1000000u);
  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeUtxo(input, output).value()),
      "828258207e2aeed860faf61b0513e9807be633a90e3260480ebc46b53ea99c497195fc29"
      "078258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb2032"
      "4c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240a1581c6666"
      "6666666666666666666666666666666666666666666666666666a143666f6f1a000f424"
      "0");

  output.coin_value.tokens.emplace(GetMockTokenId("bar"), 123u);
  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeUtxo(input, output).value()),
      "828258207e2aeed860faf61b0513e9807be633a90e3260480ebc46b53ea99c497195fc29"
      "078258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb2032"
      "4c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240a2581c6262"
      "6262626262626262626262626262626262626262626262626262a143626172187b581c66"
      "666666666666666666666666666666666666666666666666666666a143666f6f1a000f42"
      "40");

  output.coin_value.tokens.emplace(GetMockTokenId("baz"), UINT64_MAX);
  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeUtxo(input, output).value()),
      "828258207e2aeed860faf61b0513e9807be633a90e3260480ebc46b53ea99c497195fc29"
      "078258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599b36deb2032"
      "4c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace78539821a000f4240a2581c6262"
      "6262626262626262626262626262626262626262626262626262a243626172187b436261"
      "7a1bffffffffffffffff581c666666666666666666666666666666666666666666666666"
      "66666666a143666f6f1a000f4240");
}

TEST(CardanoTxDecoderTest, EncodeWitness) {
  CardanoTxDecoder::SerializableTxWitness witness;

  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeWitness(witness).value()),
      "a0");

  witness.vkey_witness_set.emplace_back(
      test::HexToArray<64>(kMockCardanoSignature),
      test::HexToArray<32>(kMockCardanoPubkey));

  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeWitness(witness).value()),
      "a100d9010281825820e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89"
      "adbfa7c2af58404dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7"
      "364161b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");

  witness.vkey_witness_set.emplace_back();
  witness.vkey_witness_set.back().public_key.fill(1);
  witness.vkey_witness_set.back().signature_bytes.fill(2);

  EXPECT_EQ(
      base::HexEncodeLower(CardanoTxDecoder::EncodeWitness(witness).value()),
      "a100d9010282825820010101010101010101010101010101010101010101010101010101"
      "010101010158400202020202020202020202020202020202020202020202020202020202"
      "020202020202020202020202020202020202020202020202020202020202020202020282"
      "5820e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af5840"
      "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609b"
      "ca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");
}

}  // namespace brave_wallet
