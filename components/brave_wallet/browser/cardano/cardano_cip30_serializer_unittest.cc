/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

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

CardanoTransaction GetReferenceTransaction() {
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

CardanoAddress GetMockCardanoAddress() {
  return *CardanoAddress::FromString(
      "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8"
      "cc3sq835lu7drv2xwl2wywfgse35a3x");
}
}  // namespace

TEST(CardanoCip30SerializerTest, SerializedSignPayload) {
  EXPECT_EQ(
      "846A5369676E6174757265315882A30127045839019493315CD92EB5D8C4304E67B7E16A"
      "E36D61D34502694657811A2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628C"
      "EFA9C4725167616464726573735839019493315CD92EB5D8C4304E67B7E16AE36D61D345"
      "02694657811A2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251"
      "40456272617665",
      base::HexEncode(CardanoCip30Serializer::SerializedSignPayload(
          GetMockCardanoAddress(), base::byte_span_from_cstring("brave"))));
}

TEST(CardanoCip30SerializerTest, SerializeSignedDataKey) {
  EXPECT_EQ(
      "A50101025839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E33"
      "7B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251032720062146707562"
      "6B6579",
      base::HexEncode(CardanoCip30Serializer::SerializeSignedDataKey(
          GetMockCardanoAddress(), base::byte_span_from_cstring("pubkey"))));
}

TEST(CardanoCip30SerializerTest, SerializeSignedDataSignature) {
  EXPECT_EQ(
      "845882A30127045839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A"
      "2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251676164647265"
      "73735839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E337B62"
      "CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251A166686173686564F44562"
      "72617665497369676E6174757265",
      base::HexEncode(CardanoCip30Serializer::SerializeSignedDataSignature(
          GetMockCardanoAddress(), base::byte_span_from_cstring("brave"),
          base::byte_span_from_cstring("signature"))));
}

TEST(CardanoCip30SerializerTest, SerializeAmount) {
  auto serialized = CardanoCip30Serializer::SerializeAmount(2000000u);
  EXPECT_EQ(serialized, "1a001e8480");
  EXPECT_EQ(2000000u,
            CardanoCip30Serializer::DeserializeAmount(serialized).value());
  EXPECT_FALSE(CardanoCip30Serializer::DeserializeAmount(""));

  auto max_serialized = CardanoCip30Serializer::SerializeAmount(UINT64_MAX);
  EXPECT_EQ(UINT64_MAX,
            CardanoCip30Serializer::DeserializeAmount(max_serialized).value());
}

TEST(CardanoCip30SerializerTest, SerializeUtxos) {
  std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>> utxos;
  // Utxo1
  {
    cardano_rpc::UnspentOutput output;
    output.output_index = 0;
    output.lovelace_amount = 1000000u;
    std::vector<uint8_t> tx_bytes;
    base::HexStringToBytes(
        "d9ef8dcd983c6fe996d5029e010e224bec191d0f63ff695cdab046abfd79dfbd",
        &tx_bytes);
    base::SpanWriter(base::span(output.tx_hash)).Write(tx_bytes);
    utxos.push_back(
        {CardanoAddress::FromString(
             "addr1qyx2zscdearcexdktcgq6g27jkyff65dw82w6catczfwxz2qjy"
             "nwf42y3c7ejrrekj5r2fh6kx5m9gcrmywpqxw3np5qjeh38p")
             .value(),
         std::move(output)});
  }

  // Utxo2
  {
    cardano_rpc::UnspentOutput output;
    output.output_index = 0;
    output.lovelace_amount = 2000000u;
    std::vector<uint8_t> tx_bytes;
    base::HexStringToBytes(
        "42c7b97f09cf640dcb76c7426c1181594dfc2da3aa000476aa9639bc0a131f4d",
        &tx_bytes);
    base::SpanWriter(base::span(output.tx_hash)).Write(tx_bytes);
    utxos.push_back({CardanoAddress::FromString(
                         "addr1q95842gcg7yr4uxqrr0l389msd68rgvv7cd9q9qc9f36mddy"
                         "q3v4daq49vspumzngv66wfydv2l3qsqtlwa2pvpd6vmstarkzs")
                         .value(),
                     std::move(output)});
  }

  EXPECT_EQ(
      std::vector<std::string>(
          {"82825820d9ef8dcd983c6fe996d5029e010e224bec191d0f63ff695cdab04"
           "6abfd79dfbd00825839010ca1430dcf478c99b65e100d215e958894ea8d71"
           "d4ed63abc092e309409126e4d5448e3d990c79b4a83526fab1a9b2a303d91"
           "c1019d198681a000f4240",
           "8282582042c7b97f09cf640dcb76c7426c1181594dfc2da3aa000476aa9639"
           "bc0a131f4d0082583901687aa91847883af0c018dff89cbb837471a18cf61a"
           "5014182a63adb5a4045956f4152b201e6c534335a7248d62bf10400bfbbaa0b02dd"
           "3371a001e8480"}),
      CardanoCip30Serializer::SerializeUtxos(utxos));

  // Empty
  EXPECT_EQ(std::vector<std::string>({}),
            CardanoCip30Serializer::SerializeUtxos({}));
}

TEST(CardanoCip30SerializerTest, RestoreTransaction) {
  auto tx = GetReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);
  auto deserialize_result =
      CardanoCip30Serializer::DeserializeTransaction(tx_bytes);
  EXPECT_TRUE(deserialize_result);

  EXPECT_EQ(deserialize_result->raw_bytes, tx_bytes);

  // Validate inputs
  EXPECT_EQ(deserialize_result->tx_body.inputs.size(), tx.inputs().size());
  for (size_t i = 0; i < tx.inputs().size(); i++) {
    EXPECT_EQ(deserialize_result->tx_body.inputs[i].tx_hash,
              tx.inputs()[i].utxo_outpoint.txid);
    EXPECT_EQ(deserialize_result->tx_body.inputs[i].index,
              tx.inputs()[i].utxo_outpoint.index);
  }

  // Validate outputs
  EXPECT_EQ(deserialize_result->tx_body.outputs.size(), tx.outputs().size());
  for (size_t i = 0; i < tx.outputs().size(); i++) {
    EXPECT_EQ(deserialize_result->tx_body.outputs[i].address,
              tx.outputs()[i].address);
    EXPECT_EQ(deserialize_result->tx_body.outputs[i].amount,
              tx.outputs()[i].amount);
  }
}

TEST(CardanoCip30SerializerTest, ApplySignatures) {
  auto tx = GetUnsignedReferenceTransaction();
  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  std::vector<CardanoSignMessageResult> sign_results;

  CardanoSignMessageResult sign_result1;
  sign_result1.pubkey.fill(1);
  sign_result1.signature.fill(2);
  sign_results.push_back(std::move(sign_result1));

  CardanoSignMessageResult sign_result2;
  sign_result2.pubkey.fill(3);
  sign_result2.signature.fill(3);
  sign_results.push_back(std::move(sign_result2));

  auto tx_with_signatures = GetUnsignedReferenceTransaction();
  std::vector<CardanoTransaction::TxWitness> witnesses;
  for (const auto& sign_result : sign_results) {
    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));

    span_writer.Write(sign_result.pubkey);
    span_writer.Write(sign_result.signature);
    witnesses.push_back(witness);
  }
  tx_with_signatures.SetWitnesses(witnesses);

  EXPECT_EQ(
      CardanoCip30Serializer::ApplySignResults(tx_bytes, sign_results).value(),
      CardanoTransactionSerializer().SerializeTransaction(tx_with_signatures));
}

}  // namespace brave_wallet
