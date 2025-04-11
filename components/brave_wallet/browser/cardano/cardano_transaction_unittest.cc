/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {
namespace {
constexpr char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
constexpr char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
constexpr char kAddress1[] =
    "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc"
    "3sq835lu7drv2xwl2wywfgse35a3x";
constexpr char kAddress2[] =
    "addr1q9cwvremt6n320s2e3agq0jyq82yhrk3htsu0w426xnz5us70z4w0jgvcdkkynmm8wmds"
    "66jd9kusnjfpu6raw5fqp0sr07p5w";

}  // namespace

TEST(CardanoTransaction, Outpoint_Value) {
  CardanoTransaction::Outpoint outpoint;
  outpoint.index = 123;
  base::HexStringToSpan(kTxid1, outpoint.txid);

  auto parsed = outpoint.FromValue(outpoint.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, outpoint);
  EXPECT_EQ(parsed->index, outpoint.index);
  EXPECT_EQ(parsed->txid, outpoint.txid);
}

TEST(CardanoTransaction, TxInput_Value) {
  CardanoTransaction::TxInput input;
  input.utxo_address = *CardanoAddress::FromString(kAddress1);
  input.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input.utxo_outpoint.txid);
  input.utxo_value = 555666777;

  auto parsed = input.FromValue(input.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, input);
  EXPECT_EQ(parsed->utxo_address, input.utxo_address);
  EXPECT_EQ(parsed->utxo_outpoint, input.utxo_outpoint);
}

TEST(CardanoTransaction, TxInput_FromRpcUtxo) {
  cardano_rpc::UnspentOutput rpc_utxo;
  rpc_utxo.tx_hash = test::HexToArray<32>(
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98");
  rpc_utxo.output_index = 1;
  rpc_utxo.lovelace_amount = 555;

  auto input = CardanoTransaction::TxInput::FromRpcUtxo(
      *CardanoAddress::FromString(kAddress1), rpc_utxo);
  ASSERT_TRUE(input);

  EXPECT_EQ(input->utxo_address, *CardanoAddress::FromString(kAddress1));
  EXPECT_EQ(input->utxo_outpoint.index, 1u);
  EXPECT_EQ(base::HexEncode(input->utxo_outpoint.txid),
            "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98");
  EXPECT_EQ(input->utxo_value, 555u);
}

TEST(CardanoTransaction, TxOutput_Value) {
  CardanoTransaction::TxOutput output;
  output.address = *CardanoAddress::FromString(kAddress2);
  output.amount = 555666777;

  auto parsed = output.FromValue(output.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, output);
  EXPECT_EQ(parsed->address, output.address);
  EXPECT_EQ(parsed->amount, output.amount);
}

TEST(CardanoTransaction, Value) {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress2);
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.AddInput(std::move(input2));

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(kAddress1);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(kAddress2);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));

  tx.set_to(*CardanoAddress::FromString(kAddress1));
  tx.set_amount(12345);
  tx.set_invalid_after(777);
  tx.set_sending_max_amount(true);

  auto parsed = tx.FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->inputs(), tx.inputs());
  EXPECT_EQ(parsed->outputs(), tx.outputs());
  EXPECT_EQ(parsed->to(), tx.to());
  EXPECT_EQ(parsed->amount(), tx.amount());
  EXPECT_EQ(parsed->invalid_after(), tx.invalid_after());
  EXPECT_EQ(parsed->sending_max_amount(), tx.sending_max_amount());
}

TEST(CardanoTransaction, IsSigned) {
  CardanoTransaction tx;
  EXPECT_FALSE(tx.IsSigned());

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  tx.AddInput(std::move(input1));

  EXPECT_FALSE(tx.IsSigned());
  tx.SetWitnesses({CardanoTransaction::TxWitness()});
  EXPECT_TRUE(tx.IsSigned());

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress2);
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;

  tx.AddInput(std::move(input2));

  EXPECT_FALSE(tx.IsSigned());
  tx.SetWitnesses(
      {CardanoTransaction::TxWitness(), CardanoTransaction::TxWitness()});
  EXPECT_TRUE(tx.IsSigned());
}

TEST(CardanoTransaction, TotalInputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.TotalInputsAmount(), 0u);

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  tx.AddInput(std::move(input1));
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u);

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress2);
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.AddInput(std::move(input2));
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u + 555u);
}

TEST(CardanoTransaction, TotalOutputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.TotalOutputsAmount(), 0u);

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(kAddress1);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));
  EXPECT_EQ(tx.TotalOutputsAmount(), 5u);

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(kAddress2);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));
  EXPECT_EQ(tx.TotalOutputsAmount(), 50u + 5u);
}

TEST(CardanoTransaction, EffectiveFeeAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.EffectiveFeeAmount(), 0u);

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  tx.AddInput(std::move(input1));
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u);

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress2);
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.AddInput(std::move(input2));
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u);

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(kAddress1);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u - 5u);

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(kAddress2);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u - 5u - 50u);
}

}  // namespace brave_wallet
