/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"

#include <utility>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

// https://adastat.net/transactions/a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba
TEST(CardanoSerializerTest, ReferenceTransaction) {
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
  tx.AddOutput(std::move(output2));

  CardanoTransaction::TxWitness witness;
  witness.witness_bytes = test::HexToArray<96>(
      "e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af"
      "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161"
      "b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");

  tx.set_invalid_after(149770436);

  EXPECT_EQ(HexEncodeLower(CardanoSerializer::GetTxHash(tx)),
            "a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba");

  EXPECT_EQ(CardanoSerializer::CalcTransactionSize(tx), 288u);

  tx.SetWitnesses({witness});

  EXPECT_EQ(HexEncodeLower(CardanoSerializer::SerializeTransaction(tx)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
            "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
            "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
            "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");

  tx.SetWitnesses({CardanoTransaction::TxWitness::DummyTxWitness()});
  EXPECT_EQ(HexEncodeLower(CardanoSerializer::SerializeTransaction(tx)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a100818258200000000000000000"
            "000000000000000000000000000000000000000000000000584000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000f5f6");

  tx.SetWitnesses({});
  EXPECT_EQ(HexEncodeLower(CardanoSerializer::SerializeTransaction(tx)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a100818158600000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "00000000000000000000000000000000000000000000f5f6");
}

}  // namespace brave_wallet
