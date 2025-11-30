/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

#include <limits>
#include <utility>

#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

cardano_rpc::EpochParameters GetReferenceEpochParameters() {
  cardano_rpc::EpochParameters epoch_parameters;
  epoch_parameters.min_fee_coefficient = 44;
  epoch_parameters.min_fee_constant = 155381;
  epoch_parameters.coins_per_utxo_size = 4310;
  return epoch_parameters;
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

  tx.set_fee(168317u);
  tx.set_invalid_after(149770436);

  CardanoTransaction::TxWitness witness;
  witness.witness_bytes = test::HexToArray<96>(
      "e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af"
      "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161"
      "b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");

  tx.SetWitnesses({witness});

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
  EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
      tx, GetReferenceEpochParameters()));

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
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();

  EXPECT_EQ(CardanoTransactionSerializer().CalcMinTransactionFee(
                GetReferenceTransaction(), epoch_parameters),
            168141u);
  EXPECT_EQ(CardanoTransactionSerializer().CalcMinTransactionFee(
                GetNullTransaction(), epoch_parameters),
            162201u);
}

TEST(CardanoTransactionSerializerTest, CalcMinAdaRequired) {
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();
  CardanoTransactionSerializer serializer;

  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/tx-construction/test/fees/fees.test.ts#L84
  CardanoTransaction::TxOutput output;
  output.address = *CardanoAddress::FromString(
      "addr_"
      "test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3jcu5d8ps7zex2k2xt3uq"
      "xgjqnnj83ws8lhrn648jjxtwq2ytjqp");
  output.amount = 0;

  EXPECT_EQ(serializer.CalcMinAdaRequired(output, epoch_parameters), 969750u);

  // Amount matches min value.
  output.amount = 969750u;
  EXPECT_EQ(serializer.CalcMinAdaRequired(output, epoch_parameters), 969750u);

  // Amount is slightly less than min value -> still same min value.
  output.amount = 960000u;
  EXPECT_EQ(serializer.CalcMinAdaRequired(output, epoch_parameters), 969750u);

  // Amount is slightly larger than min value -> still same min value.
  output.amount = 1000000u;
  EXPECT_EQ(serializer.CalcMinAdaRequired(output, epoch_parameters), 969750u);

  // Larger amount would need 9 bytes (vs 5 bytes) and then larger min value.
  output.amount = 5000000000;
  EXPECT_EQ(serializer.CalcMinAdaRequired(output, epoch_parameters), 986990u);

  // Unexpectedly large `coins_per_utxo_size` fails with no overflow.
  epoch_parameters.coins_per_utxo_size =
      std::numeric_limits<uint64_t>::max() / 2;
  EXPECT_FALSE(serializer.CalcMinAdaRequired(output, epoch_parameters));
}

TEST(CardanoTransactionSerializerTest, ValidateMinValue) {
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();
  CardanoTransactionSerializer serializer;

  // https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/tx-construction/test/fees/fees.test.ts#L84
  CardanoTransaction::TxOutput output;
  output.address = *CardanoAddress::FromString(
      "addr_"
      "test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3jcu5d8ps7zex2k2xt3uq"
      "xgjqnnj83ws8lhrn648jjxtwq2ytjqp");
  output.amount = 0;
  // Zero amount fails.
  EXPECT_FALSE(serializer.ValidateMinValue(output, epoch_parameters));

  // Amount matches min value.
  output.amount = 969750u;
  EXPECT_TRUE(serializer.ValidateMinValue(output, epoch_parameters));

  // Amount is slightly less than min value - validation fails.
  output.amount = 969740u;
  EXPECT_FALSE(serializer.ValidateMinValue(output, epoch_parameters));

  // Amount is slightly larger than min value - validation succeeds.
  output.amount = 1000000u;
  EXPECT_TRUE(serializer.ValidateMinValue(output, epoch_parameters));

  // Very large amount succeeds.
  output.amount = 5000000000;
  EXPECT_TRUE(serializer.ValidateMinValue(output, epoch_parameters));

  // Unexpectedly large `coins_per_utxo_size` fails with no overflow.
  epoch_parameters.coins_per_utxo_size =
      std::numeric_limits<uint64_t>::max() / 2;
  EXPECT_FALSE(serializer.ValidateMinValue(output, epoch_parameters));
}

TEST(CardanoTransactionSerializerTest, ValidateAmounts) {
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();

  CardanoTransaction valid_tx;

  CardanoTransaction::TxInput input1;
  input1.utxo_value = 969750u;
  valid_tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2;
  input2.utxo_value = 2000000u;
  valid_tx.AddInput(std::move(input2));

  CardanoTransaction::TxInput input3;
  input3.utxo_value = 3000000u;
  valid_tx.AddInput(std::move(input3));

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(
      "addr_"
      "test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3jcu5d8ps7zex2k2xt3uq"
      "xgjqnnj83ws8lhrn648jjxtwq2ytjqp");
  output1.amount = 969750u;
  valid_tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(
      "addr_"
      "test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3jcu5d8ps7zex2k2xt3uq"
      "xgjqnnj83ws8lhrn648jjxtwq2ytjqp");
  output2.amount = 4500000;
  valid_tx.AddOutput(std::move(output2));

  valid_tx.set_fee(500000);

  EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(valid_tx,
                                                            epoch_parameters));

  {
    CardanoTransaction tx = valid_tx;
    tx.inputs_[0].utxo_value++;
    EXPECT_FALSE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));

    tx.inputs_[1].utxo_value--;
    EXPECT_TRUE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));

    tx.outputs_[0].amount++;
    EXPECT_FALSE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));

    tx.outputs_[1].amount--;
    EXPECT_TRUE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));

    tx.fee_++;
    EXPECT_FALSE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));

    tx.fee_ -= 2;
    EXPECT_FALSE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));
  }

  {
    CardanoTransaction tx = valid_tx;
    tx.inputs_[0].utxo_value = std::numeric_limits<uint64_t>::max();
    tx.inputs_[1].utxo_value = std::numeric_limits<uint64_t>::max();
    tx.inputs_[2].utxo_value = std::numeric_limits<uint64_t>::max();

    tx.outputs_[0].amount = std::numeric_limits<int64_t>::max();
    tx.outputs_[1].amount = std::numeric_limits<int64_t>::max();
    tx.fee_ = std::numeric_limits<uint64_t>::max();

    // inputs = outputs + fee, but overflows and fails.
    EXPECT_FALSE(
        CardanoTransactionSerializer::ValidateAmounts(tx, epoch_parameters));
  }
}

TEST(CardanoTransactionSerializerTest, AdjustFeeAndOutputsForTx) {
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();

  CardanoTransaction base_tx;

  CardanoTransaction::TxInput input1;
  input1.utxo_value = 1000000u;
  base_tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2;
  input2.utxo_value = 2000000u;
  base_tx.AddInput(std::move(input2));

  CardanoTransaction::TxInput input3;
  input3.utxo_value = 3000000u;
  base_tx.AddInput(std::move(input3));

  CardanoTransaction::TxOutput output;
  output.address = *CardanoAddress::FromString(kMockCardanoAddress1);
  base_tx.AddOutput(std::move(output));

  {
    auto tx_no_change = base_tx;
    tx_no_change.TargetOutput()->amount = 6000000 - 177161u;

    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_no_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 177161u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 6000000 - 177161u);
    EXPECT_EQ(found_tx->ChangeOutput(), nullptr);

    // Slightly adjust output - doesn't work as inputs outputs and fee don't
    // match and we dont's have change.
    tx_no_change.TargetOutput()->amount = 6000000u - 177161u - 1u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_no_change, epoch_parameters));
  }

  {
    auto tx_with_change = base_tx;
    CardanoTransaction::TxOutput change_output;
    change_output.address = *CardanoAddress::FromString(kMockCardanoAddress2);
    change_output.type = CardanoTransaction::TxOutputType::kChange;
    tx_with_change.AddOutput(std::move(change_output));

    tx_with_change.TargetOutput()->amount = 1000000u;

    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 180021u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 1000000u);
    EXPECT_EQ(found_tx->ChangeOutput()->amount, 4819979u);

    // Slightly adjust output - still works.
    tx_with_change.TargetOutput()->amount = 1000000u + 123u;

    found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 180021u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 1000000u + 123u);
    EXPECT_EQ(found_tx->ChangeOutput()->amount, 4819979u - 123u);

    // Adjust output so it is larger than inputs we have - failure.
    tx_with_change.TargetOutput()->amount = 10000000u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters));

    // Adjust output so it is not possible to produce change large ehough.
    tx_with_change.TargetOutput()->amount = 5500000u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters));
  }

  {
    CardanoTransaction tx_max_send = base_tx;
    tx_max_send.set_sending_max_amount(true);
    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_max_send, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 177161u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 5822839u);
    EXPECT_EQ(found_tx->ChangeOutput(), nullptr);

    // Single input is not enough to cover fee.
    CardanoTransaction::TxInput input4;
    input4.utxo_value = 100000u;
    tx_max_send.ClearInputs();
    tx_max_send.AddInput(std::move(input4));

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_max_send, epoch_parameters));

    // Single input is not enough to produce output large enough.
    CardanoTransaction::TxInput input5;
    input5.utxo_value = 1000000u;
    tx_max_send.ClearInputs();
    tx_max_send.AddInput(std::move(input5));

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_max_send, epoch_parameters));
  }
}

}  // namespace brave_wallet
