/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

#include <limits>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
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
  input.utxo_tokens[GetMockTokenId("foo")] = 12345u;

  auto parsed = input.FromValue(input.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, input);
  EXPECT_EQ(parsed->utxo_address, input.utxo_address);
  EXPECT_EQ(parsed->utxo_outpoint, input.utxo_outpoint);
  EXPECT_EQ(parsed->utxo_tokens, input.utxo_tokens);
}

TEST(CardanoTransaction, TxInput_FromRpcUtxo) {
  cardano_rpc::UnspentOutput rpc_utxo;
  rpc_utxo.address_to = *CardanoAddress::FromString(kAddress1);
  rpc_utxo.tx_hash = test::HexToArray<32>(
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98");
  rpc_utxo.output_index = 1;
  rpc_utxo.lovelace_amount = 555;

  auto input = CardanoTransaction::TxInput::FromRpcUtxo(rpc_utxo);

  EXPECT_EQ(input.utxo_address, *CardanoAddress::FromString(kAddress1));
  EXPECT_EQ(input.utxo_outpoint.index, 1u);
  EXPECT_EQ(base::HexEncode(input.utxo_outpoint.txid),
            "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98");
  EXPECT_EQ(input.utxo_value, 555u);
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

TEST(CardanoTransaction, TxWitness_Value) {
  CardanoTransaction::TxWitness witness;
  witness.public_key = test::HexToArray<32>(
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98");
  witness.signature = test::HexToArray<64>(
      "4f2a3bc19e6df1726715ab8c03fe15d848a2c9e7f28416b5e8ce397d06aad4eb"
      "deadbeefcafebabe1234567890abcdefabcdef1234567890deadbeefcafebabe");

  auto parsed = CardanoTransaction::TxWitness::FromValue(witness.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, witness);
  EXPECT_EQ(parsed->public_key, witness.public_key);
  EXPECT_EQ(parsed->signature, witness.signature);

  base::DictValue legacy_format;
  legacy_format.Set(
      "witness_bytes",
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98"
      "4f2a3bc19e6df1726715ab8c03fe15d848a2c9e7f28416b5e8ce397d06aad4eb"
      "deadbeefcafebabe1234567890abcdefabcdef1234567890deadbeefcafebabe");

  EXPECT_EQ(CardanoTransaction::TxWitness::FromValue(legacy_format), witness);
}

TEST(CardanoTransaction, Value) {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.utxo_tokens[GetMockTokenId("foo")] = 12345u;
  input1.utxo_tokens[GetMockTokenId("bar")] = 777u;
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

  CardanoTransaction::TxWitness witness1;
  witness1.public_key.fill(2);
  witness1.signature.fill(1);
  CardanoTransaction::TxWitness witness2;
  witness2.public_key.fill(4);
  witness2.signature.fill(3);
  tx.SetWitnesses({witness1, witness2});

  tx.set_to(*CardanoAddress::FromString(kAddress1));
  tx.set_amount(12345);
  tx.set_invalid_after(777);
  tx.set_sending_max_amount(true);
  tx.set_fee(1000);

  auto parsed = CardanoTransaction::FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->inputs(), tx.inputs());
  EXPECT_EQ(parsed->outputs(), tx.outputs());
  EXPECT_EQ(parsed->witnesses(), tx.witnesses());
  EXPECT_EQ(parsed->to(), tx.to());
  EXPECT_EQ(parsed->amount(), tx.amount());
  EXPECT_EQ(parsed->fee(), tx.fee());
  EXPECT_EQ(parsed->invalid_after(), tx.invalid_after());
  EXPECT_EQ(parsed->sending_max_amount(), tx.sending_max_amount());

  // Legacy format without fee.
  auto value_no_fee = tx.ToValue();
  value_no_fee.Remove("fee");
  auto parsed_no_fee = CardanoTransaction::FromValue(value_no_fee);
  EXPECT_EQ(parsed_no_fee->fee(), 555667277u);
}

TEST(CardanoTransaction, SetupChangeOutput) {
  CardanoTransaction tx;

  EXPECT_FALSE(tx.ChangeOutput());
  tx.SetupChangeOutput(*CardanoAddress::FromString(kMockCardanoAddress1));
  EXPECT_TRUE(tx.ChangeOutput());
  EXPECT_EQ(tx.ChangeOutput()->address,
            *CardanoAddress::FromString(kMockCardanoAddress1));
  EXPECT_EQ(tx.ChangeOutput()->amount, 0u);
  EXPECT_EQ(tx.ChangeOutput()->tokens, cardano_rpc::Tokens());
  EXPECT_EQ(tx.ChangeOutput()->type, CardanoTransaction::TxOutputType::kChange);
}

TEST(CardanoTransaction, TotalInputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 0u);

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  tx.AddInput(std::move(input1));
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 555666777u);

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress2);
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.AddInput(std::move(input2));
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 555666777u + 555u);

  CardanoTransaction::TxInput input3;
  input3.utxo_address = *CardanoAddress::FromString(kAddress2);
  input3.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input3.utxo_outpoint.txid);
  input3.utxo_value = std::numeric_limits<uint64_t>::max();
  tx.AddInput(std::move(input3));
  EXPECT_FALSE(tx.GetTotalInputsAmount().IsValid());
}

TEST(CardanoTransaction, TotalOutputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 0u);

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(kAddress1);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 5u);

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(kAddress2);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 50u + 5u);

  CardanoTransaction::TxOutput output3;
  output3.address = *CardanoAddress::FromString(kAddress2);
  output3.amount = std::numeric_limits<uint64_t>::max();
  tx.AddOutput(std::move(output3));
  EXPECT_FALSE(tx.GetTotalOutputsAmount().IsValid());
}

TEST(CardanoTransaction, GetTotalInputTokensAmount) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->size(), 0u);

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  input1.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input1.utxo_value = 555666777;
  tx.AddInput(input1);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->size(), 0u);

  tx.ClearInputs();
  input1.utxo_tokens[foo_token] = 4u;
  tx.AddInput(input1);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->size(), 1u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(foo_token), 4u);

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress1);
  input2.utxo_outpoint.index = 2;
  input2.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input2.utxo_value = 2;
  input2.utxo_tokens[bar_token] = 2'000'000'000'000u;
  tx.AddInput(input2);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->size(), 2u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(foo_token), 4u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(bar_token), 2'000'000'000'000u);

  CardanoTransaction::TxInput input3;
  input3.utxo_address = *CardanoAddress::FromString(kAddress1);
  input3.utxo_outpoint.index = 8;
  input3.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input3.utxo_value = 2;
  input3.utxo_tokens[foo_token] = 1u;
  input3.utxo_tokens[bar_token] = 2u;
  input3.utxo_tokens[baz_token] = 3u;
  tx.AddInput(input3);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->size(), 3u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(foo_token), 5u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(bar_token), 2'000'000'000'002u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(baz_token), 3u);

  CardanoTransaction::TxInput input4;
  input4.utxo_address = *CardanoAddress::FromString(kAddress1);
  input4.utxo_outpoint.index = 6;
  input4.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input4.utxo_value = 2;
  input4.utxo_tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddInput(input4);
  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.GetTotalInputTokensAmount());
}

TEST(CardanoTransaction, GetTotalOutputTokensAmount) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 0u);

  CardanoTransaction::TxOutput output1;
  output1.address = *CardanoAddress::FromString(kAddress1);
  output1.amount = 555666777;
  tx.AddOutput(output1);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 0u);

  tx.ClearInputs();
  output1.tokens[foo_token] = 4u;
  tx.AddOutput(output1);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 1u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(foo_token), 4u);

  CardanoTransaction::TxOutput output2;
  output2.address = *CardanoAddress::FromString(kAddress1);
  output2.amount = 2;
  output2.tokens[bar_token] = 2'000'000'000'000u;
  tx.AddOutput(output2);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 2u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(foo_token), 4u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(bar_token), 2'000'000'000'000u);

  CardanoTransaction::TxOutput output3;
  output3.address = *CardanoAddress::FromString(kAddress1);
  output3.amount = 2;
  output3.tokens[foo_token] = 1u;
  output3.tokens[bar_token] = 2u;
  output3.tokens[baz_token] = 3u;
  tx.AddOutput(output3);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 3u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(foo_token), 5u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(bar_token), 2'000'000'000'002u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(baz_token), 3u);

  CardanoTransaction::TxOutput output4;
  output4.address = *CardanoAddress::FromString(kAddress1);
  output4.amount = 2;
  output4.tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddOutput(output4);
  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.GetTotalOutputTokensAmount());
}

TEST(CardanoTransaction, GetInputAddresses) {
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

  CardanoTransaction::TxInput input3;
  input3.utxo_address = *CardanoAddress::FromString(kAddress2);
  input3.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input3.utxo_outpoint.txid);
  input3.utxo_value = std::numeric_limits<uint64_t>::max();
  tx.AddInput(std::move(input3));

  auto addresses = tx.GetInputAddresses();
  EXPECT_THAT(tx.GetInputAddresses(),
              testing::ElementsAre(*CardanoAddress::FromString(kAddress2),
                                   *CardanoAddress::FromString(kAddress1)));
}

TEST(CardanoTransaction, EnsureTokensInChangeOutput) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  CardanoTransaction tx;

  // No tokens.
  EXPECT_TRUE(tx.EnsureTokensInChangeOutput());

  CardanoTransaction::TxInput input1;
  input1.utxo_address = *CardanoAddress::FromString(kAddress1);
  input1.utxo_outpoint.index = 123;
  input1.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input1.utxo_value = 555666777;
  input1.utxo_tokens[foo_token] = 4u;
  tx.AddInput(input1);

  CardanoTransaction::TxInput input2;
  input2.utxo_address = *CardanoAddress::FromString(kAddress1);
  input2.utxo_outpoint.index = 2;
  input2.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input2.utxo_value = 2;
  input2.utxo_tokens[bar_token] = 2'000'000'000'000u;
  tx.AddInput(input2);

  CardanoTransaction::TxInput input3;
  input3.utxo_address = *CardanoAddress::FromString(kAddress1);
  input3.utxo_outpoint.index = 8;
  input3.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input3.utxo_value = 2;
  input3.utxo_tokens[foo_token] = 1u;
  input3.utxo_tokens[bar_token] = 2u;
  input3.utxo_tokens[baz_token] = 3u;
  tx.AddInput(input3);

  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(foo_token), 5u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(bar_token), 2'000'000'000'002u);
  EXPECT_EQ(tx.GetTotalInputTokensAmount()->at(baz_token), 3u);

  // No change output.
  EXPECT_FALSE(tx.EnsureTokensInChangeOutput());
  tx.SetupChangeOutput(*CardanoAddress::FromString(kAddress2));

  EXPECT_EQ(tx.ChangeOutput()->tokens.size(), 0u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 0u);

  EXPECT_TRUE(tx.EnsureTokensInChangeOutput());

  EXPECT_EQ(tx.ChangeOutput()->tokens.size(), 3u);
  EXPECT_EQ(tx.ChangeOutput()->tokens.at(foo_token), 5u);
  EXPECT_EQ(tx.ChangeOutput()->tokens.at(bar_token), 2'000'000'000'002u);
  EXPECT_EQ(tx.ChangeOutput()->tokens.at(baz_token), 3u);

  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->size(), 3u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(foo_token), 5u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(bar_token), 2'000'000'000'002u);
  EXPECT_EQ(tx.GetTotalOutputTokensAmount()->at(baz_token), 3u);

  CardanoTransaction::TxInput input4;
  input4.utxo_outpoint.index = 14;
  input4.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input4.utxo_address = *CardanoAddress::FromString(kAddress1);
  input4.utxo_value = 2;
  input4.utxo_tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddInput(input4);

  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.EnsureTokensInChangeOutput());
}

}  // namespace brave_wallet
