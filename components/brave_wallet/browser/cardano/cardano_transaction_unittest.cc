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

  // Missing or malformed fields are rejected.
  auto no_txid = outpoint.ToValue();
  no_txid.Remove("txid");
  EXPECT_FALSE(CardanoTransaction::Outpoint::FromValue(no_txid));

  auto bad_txid = outpoint.ToValue();
  bad_txid.Set("txid", "zz");
  EXPECT_FALSE(CardanoTransaction::Outpoint::FromValue(bad_txid));

  auto no_index = outpoint.ToValue();
  no_index.Remove("index");
  EXPECT_FALSE(CardanoTransaction::Outpoint::FromValue(no_index));
}

TEST(CardanoTransaction, TxInput_Value) {
  CardanoTransaction::TxInput input(*CardanoAddress::FromString(kAddress1));
  input.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input.utxo_outpoint.txid);
  input.coin_value.lovelace_amount = 555666777;
  input.coin_value.tokens[GetMockTokenId("foo")] = 12345u;

  auto parsed = input.FromValue(input.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, input);
  EXPECT_EQ(parsed->utxo_address, input.utxo_address);
  EXPECT_EQ(parsed->utxo_outpoint, input.utxo_outpoint);
  EXPECT_EQ(parsed->coin_value, input.coin_value);

  // Missing or malformed fields are rejected.
  auto no_address = input.ToValue();
  no_address.Remove("utxo_address");
  EXPECT_FALSE(CardanoTransaction::TxInput::FromValue(no_address));

  auto no_outpoint = input.ToValue();
  no_outpoint.Remove("utxo_outpoint");
  EXPECT_FALSE(CardanoTransaction::TxInput::FromValue(no_outpoint));

  auto bad_value = input.ToValue();
  bad_value.Set("utxo_value", "abc");
  EXPECT_FALSE(CardanoTransaction::TxInput::FromValue(bad_value));

  auto no_tokens = input.ToValue();
  no_tokens.Remove("utxo_tokens");
  EXPECT_FALSE(CardanoTransaction::TxInput::FromValue(no_tokens));
}

TEST(CardanoTransaction, TxInput_FromRpcUtxo) {
  cardano_rpc::UnspentOutput rpc_utxo(*CardanoAddress::FromString(kAddress1));
  rpc_utxo.tx_hash = test::HexToArray<32>(
      "f80875bfaa0726fadc0068cca851f3252762670df345e6c7a483fe841af98e98");
  rpc_utxo.output_index = 1;
  rpc_utxo.coin_value.lovelace_amount = 555;

  auto input = CardanoTransaction::TxInput::FromRpcUtxo(rpc_utxo);

  EXPECT_EQ(input.utxo_address, *CardanoAddress::FromString(kAddress1));
  EXPECT_EQ(input.utxo_outpoint.index, 1u);
  EXPECT_EQ(base::HexEncode(input.utxo_outpoint.txid),
            "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98");
  EXPECT_EQ(input.coin_value.lovelace_amount, 555u);
}

TEST(CardanoTransaction, TxOutput_Value) {
  CardanoTransaction::TxOutput output(*CardanoAddress::FromString(kAddress2));
  output.coin_value.lovelace_amount = 555666777;

  auto parsed = output.FromValue(output.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, output);
  EXPECT_EQ(parsed->address, output.address);
  EXPECT_EQ(parsed->coin_value.lovelace_amount,
            output.coin_value.lovelace_amount);

  // Missing or malformed fields are rejected.
  auto no_address = output.ToValue();
  no_address.Remove("address");
  EXPECT_FALSE(CardanoTransaction::TxOutput::FromValue(no_address));

  auto no_type = output.ToValue();
  no_type.Remove("type");
  EXPECT_FALSE(CardanoTransaction::TxOutput::FromValue(no_type));

  auto bogus_type = output.ToValue();
  bogus_type.Set("type", "bogus");
  EXPECT_FALSE(CardanoTransaction::TxOutput::FromValue(bogus_type));

  auto bad_amount = output.ToValue();
  bad_amount.Set("amount", "abc");
  EXPECT_FALSE(CardanoTransaction::TxOutput::FromValue(bad_amount));

  auto no_tokens = output.ToValue();
  no_tokens.Remove("tokens");
  EXPECT_FALSE(CardanoTransaction::TxOutput::FromValue(no_tokens));
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

  // Missing or malformed fields are rejected.
  auto no_public_key = witness.ToValue();
  no_public_key.Remove("public_key");
  EXPECT_FALSE(CardanoTransaction::TxWitness::FromValue(no_public_key));

  auto bad_signature = witness.ToValue();
  bad_signature.Set("signature", "zz");
  EXPECT_FALSE(CardanoTransaction::TxWitness::FromValue(bad_signature));
}

TEST(CardanoTransaction, Value) {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input1(*CardanoAddress::FromString(kAddress1));
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.coin_value.lovelace_amount = 555666777;
  input1.coin_value.tokens[GetMockTokenId("foo")] = 12345u;
  input1.coin_value.tokens[GetMockTokenId("bar")] = 777u;
  tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2(*CardanoAddress::FromString(kAddress2));
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.coin_value.lovelace_amount = 555;
  tx.AddInput(std::move(input2));

  CardanoTransaction::TxOutput output1(*CardanoAddress::FromString(kAddress1));
  output1.coin_value.lovelace_amount = 5;
  output1.type = CardanoTransaction::TxOutputType::kTarget;
  tx.AddOutput(std::move(output1));

  CardanoTransaction::TxOutput output2(*CardanoAddress::FromString(kAddress2));
  output2.coin_value.lovelace_amount = 50;
  output2.type = CardanoTransaction::TxOutputType::kChange;
  tx.AddOutput(std::move(output2));

  CardanoTransaction::TxWitness witness1;
  witness1.public_key.fill(2);
  witness1.signature.fill(1);
  CardanoTransaction::TxWitness witness2;
  witness2.public_key.fill(4);
  witness2.signature.fill(3);
  tx.SetWitnesses({witness1, witness2});

  tx.set_invalid_after(777);
  tx.set_fee(1000);

  auto parsed = CardanoTransaction::FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->inputs(), tx.inputs());
  EXPECT_EQ(parsed->outputs(), tx.outputs());
  EXPECT_EQ(parsed->witnesses(), tx.witnesses());
  EXPECT_EQ(parsed->fee(), tx.fee());
  EXPECT_EQ(parsed->invalid_after(), tx.invalid_after());
  auto to_address = parsed->GetToAddress();
  ASSERT_TRUE(to_address);
  EXPECT_EQ(to_address->ToString(), kAddress1);
  EXPECT_FALSE(parsed->IsSendTokenTransaction());

  // Legacy format without fee.
  auto value_no_fee = tx.ToValue();
  value_no_fee.Remove("fee");
  auto parsed_no_fee = CardanoTransaction::FromValue(value_no_fee);
  ASSERT_TRUE(parsed_no_fee);
  EXPECT_EQ(parsed_no_fee->fee(), 555667277u);

  // Legacy format with outputs exceeding inputs overflows the fee.
  auto value_bad_fee = tx.ToValue();
  value_bad_fee.Remove("fee");
  value_bad_fee.FindList("inputs")->clear();
  EXPECT_FALSE(CardanoTransaction::FromValue(value_bad_fee));

  // Missing or malformed fields are rejected.
  for (const char* key : {"inputs", "outputs", "witnesses", "invalid_after"}) {
    auto value = tx.ToValue();
    value.Remove(key);
    EXPECT_FALSE(CardanoTransaction::FromValue(value)) << key;
  }

  for (const char* key : {"inputs", "outputs", "witnesses"}) {
    auto value = tx.ToValue();
    value.FindList(key)->front() = base::Value("not a dict");
    EXPECT_FALSE(CardanoTransaction::FromValue(value)) << key;
  }
}

TEST(CardanoTransaction, SetupTargetOutput) {
  CardanoTransaction tx;

  EXPECT_FALSE(tx.TargetOutput());
  tx.SetupTargetOutput(*CardanoAddress::FromString(kAddress1));
  ASSERT_TRUE(tx.TargetOutput());
  EXPECT_EQ(tx.TargetOutput()->address, *CardanoAddress::FromString(kAddress1));
  EXPECT_EQ(tx.TargetOutput()->coin_value, cardano_rpc::CoinValue());
  EXPECT_EQ(tx.TargetOutput()->type, CardanoTransaction::TxOutputType::kTarget);
}

TEST(CardanoTransaction, SetupChangeOutput) {
  CardanoTransaction tx;

  EXPECT_FALSE(tx.ChangeOutput());
  tx.SetupChangeOutput(*CardanoAddress::FromString(kMockCardanoAddress1));
  ASSERT_TRUE(tx.ChangeOutput());
  EXPECT_EQ(tx.ChangeOutput()->address,
            *CardanoAddress::FromString(kMockCardanoAddress1));
  EXPECT_EQ(tx.ChangeOutput()->coin_value, cardano_rpc::CoinValue());
  EXPECT_EQ(tx.ChangeOutput()->type, CardanoTransaction::TxOutputType::kChange);
}

TEST(CardanoTransaction, TotalInputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 0u);

  CardanoTransaction::TxInput input1(*CardanoAddress::FromString(kAddress1));
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.coin_value.lovelace_amount = 555666777;
  tx.AddInput(std::move(input1));
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 555666777u);

  CardanoTransaction::TxInput input2(*CardanoAddress::FromString(kAddress2));
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.coin_value.lovelace_amount = 555;
  tx.AddInput(std::move(input2));
  EXPECT_EQ(tx.GetTotalInputsAmount().ValueOrDie(), 555666777u + 555u);

  CardanoTransaction::TxInput input3(*CardanoAddress::FromString(kAddress2));
  input3.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input3.utxo_outpoint.txid);
  input3.coin_value.lovelace_amount = std::numeric_limits<uint64_t>::max();
  tx.AddInput(std::move(input3));
  EXPECT_FALSE(tx.GetTotalInputsAmount().IsValid());
}

TEST(CardanoTransaction, TotalOutputsAmount) {
  CardanoTransaction tx;
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 0u);

  CardanoTransaction::TxOutput output1(*CardanoAddress::FromString(kAddress1));
  output1.coin_value.lovelace_amount = 5;
  tx.AddOutput(std::move(output1));
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 5u);

  CardanoTransaction::TxOutput output2(*CardanoAddress::FromString(kAddress2));
  output2.coin_value.lovelace_amount = 50;
  tx.AddOutput(std::move(output2));
  EXPECT_EQ(tx.GetTotalOutputsAmount().ValueOrDie(), 50u + 5u);

  CardanoTransaction::TxOutput output3(*CardanoAddress::FromString(kAddress2));
  output3.coin_value.lovelace_amount = std::numeric_limits<uint64_t>::max();
  tx.AddOutput(std::move(output3));
  EXPECT_FALSE(tx.GetTotalOutputsAmount().IsValid());
}

TEST(CardanoTransaction, GetTotalInputTokensAmount) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  CardanoTransaction tx;
  {
    auto tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 0u);
  }

  CardanoTransaction::TxInput input1(*CardanoAddress::FromString(kAddress1));
  input1.utxo_outpoint.index = 123;
  input1.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input1.coin_value.lovelace_amount = 555666777;
  tx.AddInput(input1);
  {
    auto tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 0u);
  }

  tx.ClearInputs();
  input1.coin_value.tokens[foo_token] = 4u;
  tx.AddInput(input1);
  {
    auto tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 1u);
    EXPECT_EQ(tokens->at(foo_token), 4u);
  }

  CardanoTransaction::TxInput input2(*CardanoAddress::FromString(kAddress1));
  input2.utxo_outpoint.index = 2;
  input2.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input2.coin_value.lovelace_amount = 2;
  input2.coin_value.tokens[bar_token] = 2'000'000'000'000u;
  tx.AddInput(input2);
  {
    auto tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 2u);
    EXPECT_EQ(tokens->at(foo_token), 4u);
    EXPECT_EQ(tokens->at(bar_token), 2'000'000'000'000u);
  }

  CardanoTransaction::TxInput input3(*CardanoAddress::FromString(kAddress1));
  input3.utxo_outpoint.index = 8;
  input3.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input3.coin_value.lovelace_amount = 2;
  input3.coin_value.tokens[foo_token] = 1u;
  input3.coin_value.tokens[bar_token] = 2u;
  input3.coin_value.tokens[baz_token] = 3u;
  tx.AddInput(input3);
  {
    auto tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 3u);
    EXPECT_EQ(tokens->at(foo_token), 5u);
    EXPECT_EQ(tokens->at(bar_token), 2'000'000'000'002u);
    EXPECT_EQ(tokens->at(baz_token), 3u);
  }

  CardanoTransaction::TxInput input4(*CardanoAddress::FromString(kAddress1));
  input4.utxo_outpoint.index = 6;
  input4.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input4.coin_value.lovelace_amount = 2;
  input4.coin_value.tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddInput(input4);
  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.GetTotalInputTokensAmount());
}

TEST(CardanoTransaction, GetToAddress) {
  CardanoTransaction tx;
  EXPECT_FALSE(tx.GetToAddress());
  tx.SetupTargetOutput(*CardanoAddress::FromString(kAddress1));
  auto to_address = tx.GetToAddress();
  ASSERT_TRUE(to_address);
  EXPECT_EQ(to_address->ToString(), kAddress1);
}

TEST(CardanoTransaction, IsSendTokenTransaction) {
  CardanoTransaction tx;
  EXPECT_FALSE(tx.IsSendTokenTransaction());
  tx.SetupTargetOutput(*CardanoAddress::FromString(kAddress1));
  EXPECT_FALSE(tx.IsSendTokenTransaction());
  tx.TargetOutput()->coin_value.tokens[GetMockTokenId("foo")] = 1u;
  EXPECT_TRUE(tx.IsSendTokenTransaction());
}

TEST(CardanoTransaction, GetTotalOutputTokensAmount) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  CardanoTransaction tx;
  {
    auto tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 0u);
  }

  CardanoTransaction::TxOutput output1(*CardanoAddress::FromString(kAddress1));
  output1.coin_value.lovelace_amount = 555666777;
  tx.AddOutput(output1);
  {
    auto tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 0u);
  }

  tx.ClearInputs();
  output1.coin_value.tokens[foo_token] = 4u;
  tx.AddOutput(output1);
  {
    auto tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 1u);
    EXPECT_EQ(tokens->at(foo_token), 4u);
  }

  CardanoTransaction::TxOutput output2(*CardanoAddress::FromString(kAddress1));
  output2.coin_value.lovelace_amount = 2;
  output2.coin_value.tokens[bar_token] = 2'000'000'000'000u;
  tx.AddOutput(output2);
  {
    auto tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 2u);
    EXPECT_EQ(tokens->at(foo_token), 4u);
    EXPECT_EQ(tokens->at(bar_token), 2'000'000'000'000u);
  }

  CardanoTransaction::TxOutput output3(*CardanoAddress::FromString(kAddress1));
  output3.coin_value.lovelace_amount = 2;
  output3.coin_value.tokens[foo_token] = 1u;
  output3.coin_value.tokens[bar_token] = 2u;
  output3.coin_value.tokens[baz_token] = 3u;
  tx.AddOutput(output3);
  {
    auto tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(tokens);
    EXPECT_EQ(tokens->size(), 3u);
    EXPECT_EQ(tokens->at(foo_token), 5u);
    EXPECT_EQ(tokens->at(bar_token), 2'000'000'000'002u);
    EXPECT_EQ(tokens->at(baz_token), 3u);
  }

  CardanoTransaction::TxOutput output4(*CardanoAddress::FromString(kAddress1));
  output4.coin_value.lovelace_amount = 2;
  output4.coin_value.tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddOutput(output4);
  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.GetTotalOutputTokensAmount());
}

TEST(CardanoTransaction, GetInputAddresses) {
  CardanoTransaction tx;

  CardanoTransaction::TxInput input1(*CardanoAddress::FromString(kAddress1));
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.coin_value.lovelace_amount = 555666777;
  tx.AddInput(std::move(input1));

  CardanoTransaction::TxInput input2(*CardanoAddress::FromString(kAddress2));
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.coin_value.lovelace_amount = 555;
  tx.AddInput(std::move(input2));

  CardanoTransaction::TxInput input3(*CardanoAddress::FromString(kAddress2));
  input3.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input3.utxo_outpoint.txid);
  input3.coin_value.lovelace_amount = std::numeric_limits<uint64_t>::max();
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
  tx.SetupTargetOutput(*CardanoAddress::FromString(kAddress2));

  // No tokens.
  EXPECT_TRUE(tx.EnsureTokensInChangeOutput());

  CardanoTransaction::TxInput input1(*CardanoAddress::FromString(kAddress1));
  input1.utxo_outpoint.index = 123;
  input1.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input1.coin_value.lovelace_amount = 555666777;
  input1.coin_value.tokens[foo_token] = 4u;
  tx.AddInput(input1);

  CardanoTransaction::TxInput input2(*CardanoAddress::FromString(kAddress1));
  input2.utxo_outpoint.index = 2;
  input2.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input2.coin_value.lovelace_amount = 2;
  input2.coin_value.tokens[bar_token] = 2'000'000'000'000u;
  tx.AddInput(input2);

  CardanoTransaction::TxInput input3(*CardanoAddress::FromString(kAddress1));
  input3.utxo_outpoint.index = 8;
  input3.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input3.coin_value.lovelace_amount = 2;
  input3.coin_value.tokens[foo_token] = 1u;
  input3.coin_value.tokens[bar_token] = 2u;
  input3.coin_value.tokens[baz_token] = 3u;
  tx.AddInput(input3);

  {
    auto input_tokens = tx.GetTotalInputTokensAmount();
    ASSERT_TRUE(input_tokens);
    EXPECT_EQ(input_tokens->at(foo_token), 5u);
    EXPECT_EQ(input_tokens->at(bar_token), 2'000'000'000'002u);
    EXPECT_EQ(input_tokens->at(baz_token), 3u);
  }

  // No change output.
  EXPECT_FALSE(tx.EnsureTokensInChangeOutput());
  tx.SetupChangeOutput(*CardanoAddress::FromString(kAddress2));
  ASSERT_TRUE(tx.ChangeOutput());

  EXPECT_EQ(tx.ChangeOutput()->coin_value.tokens.size(), 0u);
  {
    auto output_tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(output_tokens);
    EXPECT_EQ(output_tokens->size(), 0u);
  }

  EXPECT_TRUE(tx.EnsureTokensInChangeOutput());

  EXPECT_EQ(tx.ChangeOutput()->coin_value.tokens.size(), 3u);
  EXPECT_EQ(tx.ChangeOutput()->coin_value.tokens.at(foo_token), 5u);
  EXPECT_EQ(tx.ChangeOutput()->coin_value.tokens.at(bar_token),
            2'000'000'000'002u);
  EXPECT_EQ(tx.ChangeOutput()->coin_value.tokens.at(baz_token), 3u);

  {
    auto output_tokens = tx.GetTotalOutputTokensAmount();
    ASSERT_TRUE(output_tokens);
    EXPECT_EQ(output_tokens->size(), 3u);
    EXPECT_EQ(output_tokens->at(foo_token), 5u);
    EXPECT_EQ(output_tokens->at(bar_token), 2'000'000'000'002u);
    EXPECT_EQ(output_tokens->at(baz_token), 3u);
  }

  CardanoTransaction::TxInput input4(*CardanoAddress::FromString(kAddress1));
  input4.utxo_outpoint.index = 14;
  input4.utxo_outpoint.txid = test::HexToArray<32>(kTxid1);
  input4.coin_value.lovelace_amount = 2;
  input4.coin_value.tokens[baz_token] = std::numeric_limits<uint64_t>::max();
  tx.AddInput(input4);

  // Sum of baz tokens overflows.
  EXPECT_FALSE(tx.EnsureTokensInChangeOutput());
}

}  // namespace brave_wallet
