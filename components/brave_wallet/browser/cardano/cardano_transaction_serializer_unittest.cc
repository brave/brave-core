/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

#include <limits>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

class ScopedUseSetTagForTesting {
 public:
  explicit ScopedUseSetTagForTesting(bool value) : value_(value) {
    CardanoTxDecoder::SetUseSetTagForTesting(value_);
  }
  ~ScopedUseSetTagForTesting() {
    CardanoTxDecoder::SetUseSetTagForTesting(!value_);
  }

 private:
  bool value_ = false;
};

cardano_rpc::EpochParameters GetReferenceEpochParameters() {
  cardano_rpc::EpochParameters epoch_parameters;
  epoch_parameters.min_fee_coefficient = 44;
  epoch_parameters.min_fee_constant = 155381;
  epoch_parameters.coins_per_utxo_size = 4310;
  return epoch_parameters;
}

CardanoTransaction GetReferenceTransactionNoTag() {
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
  witness.public_key = test::HexToArray<32>(
      "e68ca46554098776f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af");
  witness.signature = test::HexToArray<64>(
      "4dd83381fdc64b6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161"
      "b7b3609bca439b62e232731fb5290c495601cf40b358f915ade8bcff1eb7b802");

  tx.SetWitnesses({witness});

  return tx;
}

CardanoTransaction GetReferenceTransactionWithTag() {
  // https://adastat.net/transactions/0a176a7e0add815704f5b2f3bd4aee0de0c3d331585d39d3b9b0d426fab57f16
  CardanoTransaction tx;

  CardanoTransaction::TxInput input1;
  input1.utxo_outpoint.txid = test::HexToArray<32>(
      "cf36372f1d91161bb1b6ce882890ae8d6c20bd7175f9c8430acc50023075e80f");
  input1.utxo_outpoint.index = 0;
  input1.utxo_value = 1000000;
  tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2;
  input2.utxo_outpoint.txid = test::HexToArray<32>(
      "cf36372f1d91161bb1b6ce882890ae8d6c20bd7175f9c8430acc50023075e80f");
  input2.utxo_outpoint.index = 1;
  input2.utxo_value = 10369265056;
  tx.AddInput(std::move(input2));

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(
      "addr1qxazrc6avxgumuzzq7ynunf9avkl5k972d0nk0s653m2jfsmqlr2xhd2s6w6m3y3r3l"
      "ruknyu39sa2js2nygaw03ffpsj554nm");
  output1.amount = 10370089635;
  tx.AddOutput(std::move(output1));

  tx.set_fee(175421);
  tx.set_invalid_after(1000000000);

  CardanoTransaction::TxWitness witness1;
  witness1.public_key = test::HexToArray<32>(
      "b7e791e55b27e37718d213c16dba23d1b76f4a683cee4c96391602b44c5bd0e5");
  witness1.signature = test::HexToArray<64>(
      "4fe2234b679461fedc89dfc7581703ecede9f2cc4701be89cd4d35cb7dbae6c1"
      "ab1062b6b49199ced59803224a8c974946675c41c718a1e4b41bee883a936606");

  CardanoTransaction::TxWitness witness2;
  witness2.public_key = test::HexToArray<32>(
      "b7e791e55b27e37718d213c16dba23d1b76f4a683cee4c96391602b44c5bd0e5");
  witness2.signature = test::HexToArray<64>(
      "4fe2234b679461fedc89dfc7581703ecede9f2cc4701be89cd4d35cb7dbae6c1"
      "ab1062b6b49199ced59803224a8c974946675c41c718a1e4b41bee883a936606");

  tx.SetWitnesses({witness1, witness2});

  return tx;
}

}  // namespace

TEST(CardanoTransactionSerializerTest, ReferenceTransactions) {
  // https://adastat.net/transactions/0a176a7e0add815704f5b2f3bd4aee0de0c3d331585d39d3b9b0d426fab57f16
  CardanoTransaction tx_with_tag = GetReferenceTransactionWithTag();
  EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
      tx_with_tag, GetReferenceEpochParameters()));

  EXPECT_EQ(base::HexEncodeLower(
                *CardanoTransactionSerializer::GetTxHash(tx_with_tag)),
            "0a176a7e0add815704f5b2f3bd4aee0de0c3d331585d39d3b9b0d426fab57f16");

  EXPECT_EQ(
      base::HexEncodeLower(
          *CardanoTransactionSerializer::SerializeTransaction(tx_with_tag)),
      "84a400d9010282825820cf36372f1d91161bb1b6ce882890ae8d6c20bd7175f9c8430acc"
      "50023075e80f00825820cf36372f1d91161bb1b6ce882890ae8d6c20bd7175f9c8430acc"
      "50023075e80f01018182583901ba21e35d6191cdf04207893e4d25eb2dfa58be535f3b3e"
      "1aa476a9261b07c6a35daa869dadc4911c7e3e5a64e44b0eaa5054c88eb9f14a431b0000"
      "00026a1b02a3021a0002ad3d031a3b9aca00a100d9010282825820b7e791e55b27e37718"
      "d213c16dba23d1b76f4a683cee4c96391602b44c5bd0e558404fe2234b679461fedc89df"
      "c7581703ecede9f2cc4701be89cd4d35cb7dbae6c1ab1062b6b49199ced59803224a8c97"
      "4946675c41c718a1e4b41bee883a936606825820b7e791e55b27e37718d213c16dba23d1"
      "b76f4a683cee4c96391602b44c5bd0e558404fe2234b679461fedc89dfc7581703ecede9"
      "f2cc4701be89cd4d35cb7dbae6c1ab1062b6b49199ced59803224a8c974946675c41c718"
      "a1e4b41bee883a936606f5f6");

  ScopedUseSetTagForTesting scoped_use_set_tag_for_testing(false);

  // https://adastat.net/transactions/a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba
  CardanoTransaction tx_no_tag = GetReferenceTransactionNoTag();
  EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
      tx_no_tag, GetReferenceEpochParameters()));

  EXPECT_EQ(
      base::HexEncodeLower(*CardanoTransactionSerializer::GetTxHash(tx_no_tag)),
      "a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba");

  EXPECT_EQ(base::HexEncodeLower(
                *CardanoTransactionSerializer::SerializeTransaction(tx_no_tag)),
            "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
            "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
            "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
            "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
            "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
            "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
            "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
            "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
            "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");
}

TEST(CardanoTransactionSerializerTest, SerializeTransaction) {
  EXPECT_EQ(base::HexEncodeLower(*CardanoTransactionSerializer::GetTxHash(
                GetReferenceTransactionWithTag())),
            "0a176a7e0add815704f5b2f3bd4aee0de0c3d331585d39d3b9b0d426fab57f16");

  ScopedUseSetTagForTesting scoped_use_set_tag_for_testing(false);

  EXPECT_EQ(
      base::HexEncodeLower(*CardanoTransactionSerializer::SerializeTransaction(
          GetReferenceTransactionNoTag())),
      "84a40081825820a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240a"
      "dd8c71edb9250001828258390144e5e8699ab31de351be61dfeb7c220eff61d29d"
      "9c88ca9d1599b36deb20324c1f3c7c6a216e551523ff7ef4e784f3fde3606a5bac"
      "e785391a0098968082583901e057e6ff439d606a3e6c47a00b867734098461b83a"
      "d9943242b6bc04b7b276465449b932964b6173bc9f38a87677136918dc79f746c1"
      "c21d1a017286c0021a0002917d031a08ed50c4a10081825820e68ca46554098776"
      "f19f1433da96a108ea8bdda693fb1bea748f89adbfa7c2af58404dd83381fdc64b"
      "6123f193e23c983a99c979a1af44b1bda5ea15d06cf7364161b7b3609bca439b62"
      "e232731fb5290c495601cf40b358f915ade8bcff1eb7b802f5f6");
}

TEST(CardanoTransactionSerializerTest, GetTxHash) {
  EXPECT_EQ(base::HexEncodeLower(*CardanoTransactionSerializer::GetTxHash(
                GetReferenceTransactionWithTag())),
            "0a176a7e0add815704f5b2f3bd4aee0de0c3d331585d39d3b9b0d426fab57f16");

  ScopedUseSetTagForTesting scoped_use_set_tag_for_testing(false);

  EXPECT_EQ(base::HexEncodeLower(*CardanoTransactionSerializer::GetTxHash(
                GetReferenceTransactionNoTag())),
            "a634a34c535a86aa7125023e816d2fac982d530b0848dcc40738a33aca09c9ba");
}

TEST(CardanoTransactionSerializerTest, CalcMinTransactionFee) {
  cardano_rpc::EpochParameters epoch_parameters = GetReferenceEpochParameters();

  EXPECT_EQ(*CardanoTransactionSerializer::CalcMinTransactionFee(
                GetReferenceTransactionWithTag(), epoch_parameters),
            171749u);

  ScopedUseSetTagForTesting scoped_use_set_tag_for_testing(false);
  EXPECT_EQ(*CardanoTransactionSerializer::CalcMinTransactionFee(
                GetReferenceTransactionNoTag(), epoch_parameters),
            168141u);
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
  // Changing inputs, outputs or fee would fail validation.
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

  // Overflow test.
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

  // Some inputs, no change.
  {
    auto tx_no_change = base_tx;
    tx_no_change.TargetOutput()->amount = 6000000 - 168537u;

    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_no_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 168537u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 6000000 - 168537u);
    EXPECT_EQ(found_tx->ChangeOutput(), nullptr);

    // Slightly adjust output - doesn't work as inputs outputs and fee don't
    // match and we dont's have change.
    tx_no_change.TargetOutput()->amount = 6000000u - 168537u - 1u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_no_change, epoch_parameters));
  }

  // Some inputs, has change.
  {
    auto tx_with_change = base_tx;
    CardanoTransaction::TxOutput change_output;
    change_output.address = *CardanoAddress::FromString(kMockCardanoAddress2);
    change_output.type = CardanoTransaction::TxOutputType::kChange;
    tx_with_change.AddOutput(std::move(change_output));

    tx_with_change.TargetOutput()->amount = 1000000u;

    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 171397u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 1000000u);
    EXPECT_EQ(found_tx->ChangeOutput()->amount, 6000000u - 1000000u - 171397u);

    // Slightly adjust output - still works.
    tx_with_change.TargetOutput()->amount = 1000000u + 123u;

    found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 171397u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 1000000u + 123u);
    EXPECT_EQ(found_tx->ChangeOutput()->amount,
              6000000u - (1000000u + 123u) - 171397u);

    // Adjust output so it is larger than inputs we have - failure.
    tx_with_change.TargetOutput()->amount = 10000000u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters));

    // Adjust output so it is not possible to produce change large enough.
    tx_with_change.TargetOutput()->amount = 5500000u;

    EXPECT_FALSE(CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_with_change, epoch_parameters));
  }

  // Sending max amount.
  {
    CardanoTransaction tx_max_send = base_tx;
    tx_max_send.set_sending_max_amount(true);
    auto found_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
        tx_max_send, epoch_parameters);

    EXPECT_EQ(found_tx->fee(), 168537u);
    EXPECT_EQ(found_tx->inputs(), base_tx.inputs());
    EXPECT_EQ(found_tx->TargetOutput()->amount, 6000000u - 168537u);
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
