/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

#include <utility>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

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

CardanoTransaction GetNullTransaction() {
  CardanoTransaction null_tx;
  null_tx.AddInput(CardanoTransaction::TxInput());
  null_tx.AddOutput(CardanoTransaction::TxOutput());
  null_tx.SetWitnesses({CardanoTransaction::TxWitness()});
  return null_tx;
}

}  // namespace

// https://adastat.net/transactions/a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba
TEST(CardanoTransactionSerializerTest, ReferenceTransaction) {
  CardanoTransaction tx = GetReferenceTransaction();

  EXPECT_EQ(base::HexEncodeLower(CardanoTransactionSerializer().GetTxHash(tx)),
            "a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba");

  EXPECT_EQ(base::HexEncodeLower(
                CardanoTransactionSerializer().SerializeTransaction(tx)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
            "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
            "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
            "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");

  tx.SetWitnesses({});
  EXPECT_EQ(base::HexEncodeLower(
                CardanoTransactionSerializer({.use_dummy_witness_set = true})
                    .SerializeTransaction(tx)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a100818258200000000000000000"
            "000000000000000000000000000000000000000000000000584000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000f5f6");
}

TEST(CardanoTransactionSerializerTest, Options) {
  EXPECT_EQ(
      base::HexEncodeLower(CardanoTransactionSerializer().SerializeTransaction(
          GetReferenceTransaction())),
      "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
      "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
      "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
      "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
      "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
      "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
      "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
      "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
      "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");

  EXPECT_EQ(base::HexEncodeLower(
                CardanoTransactionSerializer({.use_dummy_witness_set = true})
                    .SerializeTransaction(GetReferenceTransaction())),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a100818258200000000000000000"
            "000000000000000000000000000000000000000000000000584000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000f5f6");

  EXPECT_EQ(base::HexEncodeLower(
                CardanoTransactionSerializer({.max_value_for_fee = true})
                    .SerializeTransaction(GetReferenceTransaction())),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021b7fffffffffffffff031a08ed50c4a10081825820e68ca465"
            "54098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd833"
            "81fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609b"
            "ca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");

  EXPECT_EQ(
      base::HexEncodeLower(
          CardanoTransactionSerializer({.max_value_for_change_output = true})
              .SerializeTransaction(GetReferenceTransaction())),
      "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71"
      "edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599"
      "b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace785391a0098968082"
      "583901e057e6ff439d606a3e6c47a00b867734098461b83ad9943242b6bc04b7b2764654"
      "49b932964b6173bc9f38a87677136918dc79f746c1c21d1b7fffffffffffffff021a0002"
      "917d031a08ed50c4a10081825820e68ca46554098776f19f1433da96a108ea8bdda693fb"
      "1bea748f89adbfa7c2af58404dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5"
      "ea15d06cf7364161b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff"
      "1eb7b802f5f6");

  EXPECT_EQ(
      base::HexEncodeLower(
          CardanoTransactionSerializer({.max_value_for_target_output = true})
              .SerializeTransaction(GetReferenceTransaction())),
      "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71"
      "edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d9c88ca9d1599"
      "b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bace785391b7fffffffff"
      "ffffff82583901e057e6ff439d606a3e6c47a00b867734098461b83ad9943242b6bc04b7"
      "b276465449b932964b6173bc9f38a87677136918dc79f746c1c21d1a017286c0021a0002"
      "917d031a08ed50c4a10081825820e68ca46554098776f19f1433da96a108ea8bdda693fb"
      "1bea748f89adbfa7c2af58404dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5"
      "ea15d06cf7364161b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff"
      "1eb7b802f5f6");
}

TEST(CardanoTransactionSerializerTest, SerializeTransaction) {
  EXPECT_EQ(
      base::HexEncodeLower(CardanoTransactionSerializer().SerializeTransaction(
          GetReferenceTransaction())),
      "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
      "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
      "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
      "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
      "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
      "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
      "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
      "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
      "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");

  EXPECT_EQ(
      base::HexEncodeLower(CardanoTransactionSerializer().SerializeTransaction(
          GetNullTransaction())),
      "84a400818258200000000000000000000000000000000000000000000000000000"
      "00000000000000018182400002000300a100818258200000000000000000000000"
      "000000000000000000000000000000000000000000584000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000f5f6");
}

TEST(CardanoTransactionSerializerTest, CalcTransactionSize) {
  EXPECT_EQ(CardanoTransactionSerializer().CalcTransactionSize(
                GetReferenceTransaction()),
            290u);

  EXPECT_EQ(
      CardanoTransactionSerializer().CalcTransactionSize(GetNullTransaction()),
      155u);
}

TEST(CardanoTransactionSerializerTest, GetTxHash) {
  EXPECT_EQ(base::HexEncodeLower(CardanoTransactionSerializer().GetTxHash(
                GetReferenceTransaction())),
            "a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba");

  EXPECT_EQ(base::HexEncodeLower(
                CardanoTransactionSerializer().GetTxHash(GetNullTransaction())),
            "b2ea07342a0c25200d1078cf0ea9b74942d1fb6284f812373182f0eb0270f5e6");
}

TEST(CardanoTransactionSerializerTest, CalcMinTransactionFee) {
  cardano_rpc::EpochParameters epoch_parameters;
  epoch_parameters.min_fee_coefficient = 44;
  epoch_parameters.min_fee_constant = 155381;
  EXPECT_EQ(CardanoTransactionSerializer().CalcMinTransactionFee(
                GetReferenceTransaction(), epoch_parameters),
            168141u);
  EXPECT_EQ(CardanoTransactionSerializer().CalcMinTransactionFee(
                GetNullTransaction(), epoch_parameters),
            162201u);
}

}  // namespace brave_wallet
