/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {
namespace {
const char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
const char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
const char kAddress1[] = "tb1qya3rarek59486w345v45tv6nra4fy2xxgky26x";
const char kAddress2[] = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";

}  // namespace

TEST(BitcoinTransaction, Outpoint_Value) {
  BitcoinTransaction::Outpoint outpoint;
  outpoint.index = 123;
  base::HexStringToSpan(kTxid1, outpoint.txid);

  auto parsed = outpoint.FromValue(outpoint.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, outpoint);
  EXPECT_EQ(parsed->index, outpoint.index);
  EXPECT_EQ(parsed->txid, outpoint.txid);
}

TEST(BitcoinTransaction, TxInput_Value) {
  BitcoinTransaction::TxInput input;
  input.utxo_address = kAddress1;
  input.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input.utxo_outpoint.txid);
  input.utxo_value = 555666777;
  input.script_sig = {1, 2, 3};
  input.witness = {4, 5, 6};

  auto parsed = input.FromValue(input.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, input);
  EXPECT_EQ(parsed->utxo_address, input.utxo_address);
  EXPECT_EQ(parsed->utxo_outpoint, input.utxo_outpoint);
  EXPECT_EQ(parsed->script_sig, input.script_sig);
  EXPECT_EQ(parsed->witness, input.witness);
  EXPECT_EQ(parsed->n_sequence(), 0xfffffffd);
}

TEST(BitcoinTransaction, TxInput_FromRpcUtxo) {
  const std::string rpc_utxo_json = R"(
    {
      "txid": "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98",
      "vout": 0,
      "status": {
        "confirmed": true,
        "block_height": 2474734,
        "block_hash": "000000000000000e4827189881909630974e4cc93953642f715fd86464a52808",
        "block_time": 1692873891
      },
      "value": 2407560
    }
  )";

  auto rpc_utxo = bitcoin_rpc::UnspentOutput::FromValue(base::test::ParseJson(
      std::string(json::convert_all_numbers_to_string(rpc_utxo_json, ""))));
  ASSERT_TRUE(rpc_utxo);

  auto input = BitcoinTransaction::TxInput::FromRpcUtxo(kAddress1, *rpc_utxo);
  ASSERT_TRUE(input);

  EXPECT_EQ(input->utxo_address, kAddress1);
  EXPECT_EQ(input->utxo_outpoint.index, 0u);
  EXPECT_EQ(base::HexEncode(input->utxo_outpoint.txid),
            "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98");
  EXPECT_EQ(input->utxo_value, 2407560u);
  EXPECT_TRUE(input->script_sig.empty());
  EXPECT_TRUE(input->witness.empty());
  EXPECT_EQ(input->n_sequence(), 0xfffffffd);
}

TEST(BitcoinTransaction, TxOutput_Value) {
  BitcoinTransaction::TxOutput output;
  output.address = kAddress2;
  output.amount = 555666777;

  auto parsed = output.FromValue(output.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, output);
  EXPECT_EQ(parsed->address, output.address);
  EXPECT_EQ(parsed->amount, output.amount);
}

TEST(BitcoinTransaction, Value) {
  BitcoinTransaction tx;

  tx.inputs().reserve(2);

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.script_sig = {1, 2};
  input2.witness = {4, 5};

  auto& output1 = tx.outputs().emplace_back();
  output1.address = kAddress1;
  output1.amount = 5;

  auto& output2 = tx.outputs().emplace_back();
  output2.address = kAddress2;
  output2.amount = 50;

  tx.set_to(kAddress1);
  tx.set_amount(12345);
  tx.set_locktime(777);

  auto parsed = tx.FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->inputs(), tx.inputs());
  EXPECT_EQ(parsed->outputs(), tx.outputs());
  EXPECT_EQ(parsed->to(), tx.to());
  EXPECT_EQ(parsed->amount(), tx.amount());
  EXPECT_EQ(parsed->locktime(), tx.locktime());
}

TEST(BitcoinTransaction, IsSigned) {
  BitcoinTransaction tx;
  EXPECT_FALSE(tx.IsSigned());

  tx.inputs().reserve(2);

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};
  EXPECT_TRUE(input1.IsSigned());
  EXPECT_TRUE(tx.IsSigned());

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  EXPECT_FALSE(input2.IsSigned());
  EXPECT_FALSE(tx.IsSigned());

  input2.witness = {4, 5};
  EXPECT_TRUE(input2.IsSigned());
  EXPECT_TRUE(tx.IsSigned());

  input2.script_sig = {1, 2};
  input2.witness = {};
  EXPECT_TRUE(input2.IsSigned());
  EXPECT_TRUE(tx.IsSigned());

  tx.ClearSignatures();
  EXPECT_FALSE(input1.IsSigned());
  EXPECT_FALSE(input2.IsSigned());
  EXPECT_FALSE(tx.IsSigned());
}

TEST(BitcoinTransaction, TotalInputsAmount) {
  BitcoinTransaction tx;
  EXPECT_EQ(tx.TotalInputsAmount(), 0u);

  tx.inputs().reserve(2);

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u);

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u + 555u);
}

TEST(BitcoinTransaction, TotalOutputsAmount) {
  BitcoinTransaction tx;
  EXPECT_EQ(tx.TotalOutputsAmount(), 0u);

  auto& output1 = tx.outputs().emplace_back();
  output1.address = kAddress1;
  output1.amount = 5;
  EXPECT_EQ(tx.TotalOutputsAmount(), 5u);

  auto& output2 = tx.outputs().emplace_back();
  output2.address = kAddress2;
  output2.amount = 50;
  EXPECT_EQ(tx.TotalOutputsAmount(), 50u + 5u);
}

TEST(BitcoinTransaction, EffectiveFeeAmount) {
  BitcoinTransaction tx;
  EXPECT_EQ(tx.EffectiveFeeAmount(), 0u);

  tx.inputs().reserve(2);

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u);

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u);

  auto& output1 = tx.outputs().emplace_back();
  output1.address = kAddress1;
  output1.amount = 5;
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u - 5u);

  auto& output2 = tx.outputs().emplace_back();
  output2.address = kAddress2;
  output2.amount = 50;
  EXPECT_EQ(tx.EffectiveFeeAmount(), 555666777u + 555u - 5u - 50u);
}

}  // namespace brave_wallet
