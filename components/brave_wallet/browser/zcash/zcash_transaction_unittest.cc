/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {
namespace {
const char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
const char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
const char kAddress1[] = "t1WU75sSfiPbK5ez33uuhEbd9ZD3XNCxMRj";
const char kAddress2[] = "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS";

}  // namespace

TEST(ZCashTransaction, Outpoint_Value) {
  ZCashTransaction::Outpoint outpoint;
  outpoint.index = 123;
  base::HexStringToSpan(kTxid1, outpoint.txid);

  auto parsed = outpoint.FromValue(outpoint.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, outpoint);
  EXPECT_EQ(parsed->index, outpoint.index);
  EXPECT_EQ(parsed->txid, outpoint.txid);
}

TEST(ZCashTransaction, TxInput_Value) {
  ZCashTransaction::TxInput input;
  input.utxo_address = kAddress1;
  input.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input.utxo_outpoint.txid);
  input.utxo_value = 555666777;
  input.script_sig = {1, 2, 3};
  input.script_pub_key = {4, 5, 6};

  auto parsed = input.FromValue(input.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, input);
  EXPECT_EQ(parsed->utxo_address, input.utxo_address);
  EXPECT_EQ(parsed->utxo_outpoint, input.utxo_outpoint);
  EXPECT_EQ(parsed->script_sig, input.script_sig);
  EXPECT_EQ(parsed->n_sequence, 0xffffffff);
  EXPECT_EQ(parsed->script_pub_key, input.script_pub_key);
}

TEST(ZCashTransaction, TxInput_FromRpcUtxo) {
  std::vector<uint8_t> tx_id;
  base::HexStringToBytes(
      "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98",
      &tx_id);
  auto rpc_utxo = zcash::mojom::ZCashUtxo::New(
      kAddress1, tx_id, 0u, std::vector<uint8_t>({1, 2, 3}), 2407560u, 100u);
  ASSERT_TRUE(rpc_utxo);

  auto input = ZCashTransaction::TxInput::FromRpcUtxo(kAddress1, *rpc_utxo);
  ASSERT_TRUE(input);

  EXPECT_EQ(input->utxo_address, kAddress1);
  EXPECT_EQ(input->utxo_outpoint.index, 0u);
  EXPECT_EQ(base::HexEncode(input->utxo_outpoint.txid),
            "F80875BFAA0726FADC0068CCA851F3252762670DF345E6C7A483FE841AF98E98");
  EXPECT_EQ(input->utxo_value, 2407560u);
  EXPECT_TRUE(input->script_sig.empty());
  EXPECT_FALSE(input->IsSigned());
  EXPECT_EQ(input->n_sequence, 0xffffffff);
}

TEST(ZCashTransaction, TxOutput_Value) {
  ZCashTransaction::TxOutput output;
  output.address = kAddress2;
  output.script_pubkey.assign({0, 1, 2, 7});
  output.amount = 555666777;

  auto parsed = output.FromValue(output.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, output);
  EXPECT_EQ(parsed->address, output.address);
  EXPECT_EQ(parsed->script_pubkey, output.script_pubkey);
  EXPECT_EQ(parsed->amount, output.amount);
}

TEST(ZCashTransaction, Value) {
  ZCashTransaction tx;

  ZCashTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  tx.transparent_part().inputs.push_back(std::move(input1));

  ZCashTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.script_sig = {1, 2};
  tx.transparent_part().inputs.push_back(std::move(input2));

  ZCashTransaction::TxOutput output1;
  output1.address = kAddress1;
  output1.script_pubkey = ZCashAddressToScriptPubkey(kAddress1, true);
  output1.amount = 5;
  tx.transparent_part().outputs.push_back(std::move(output1));

  ZCashTransaction::TxOutput output2;
  output2.address = kAddress2;
  output2.script_pubkey = ZCashAddressToScriptPubkey(kAddress2, true);
  output2.amount = 50;
  tx.transparent_part().outputs.push_back(std::move(output2));

  OrchardMemo memo;
  memo.fill(2);

  ZCashTransaction::OrchardOutput orchard_output;
  orchard_output.value = 100;
  orchard_output.addr.fill(2);
  orchard_output.memo = memo;
  tx.orchard_part().outputs.push_back(std::move(orchard_output));

  tx.set_to(kAddress1);
  tx.set_amount(12345);
  tx.set_locktime(777);
  tx.set_memo(memo);

  auto parsed = tx.FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->transparent_part().inputs, tx.transparent_part().inputs);
  EXPECT_EQ(parsed->transparent_part().outputs, tx.transparent_part().outputs);
  EXPECT_EQ(parsed->orchard_part().outputs, tx.orchard_part().outputs);
  EXPECT_EQ(parsed->to(), tx.to());
  EXPECT_EQ(parsed->amount(), tx.amount());
  EXPECT_EQ(parsed->locktime(), tx.locktime());
  EXPECT_EQ(parsed->memo(), tx.memo());
}

TEST(ZCashTransaction, IsSigned) {
  ZCashTransaction tx;
  EXPECT_FALSE(tx.IsTransparentPartSigned());

  ZCashTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  EXPECT_TRUE(input1.IsSigned());
  tx.transparent_part().inputs.push_back(std::move(input1));
  EXPECT_TRUE(tx.IsTransparentPartSigned());

  ZCashTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  EXPECT_FALSE(input2.IsSigned());

  input2.script_sig = {1, 2};
  EXPECT_TRUE(input2.IsSigned());
  tx.transparent_part().inputs.push_back(std::move(input2));
  EXPECT_TRUE(tx.IsTransparentPartSigned());

  EXPECT_TRUE(tx.transparent_part().inputs[0].IsSigned());
  EXPECT_TRUE(tx.transparent_part().inputs[1].IsSigned());
  EXPECT_TRUE(tx.IsTransparentPartSigned());
}

TEST(ZCashTransaction, TotalInputsAmount) {
  ZCashTransaction tx;
  EXPECT_EQ(tx.TotalInputsAmount(), 0u);

  ZCashTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  tx.transparent_part().inputs.push_back(std::move(input1));
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u);

  ZCashTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.transparent_part().inputs.push_back(input2);
  EXPECT_EQ(tx.TotalInputsAmount(), 555666777u + 555u);
}

TEST(ZCashTransaction, ShieldedOutputs) {
  {
    ZCashTransaction::OrchardOutput output;
    output.addr.fill(1);

    OrchardMemo memo;
    memo.fill(2);
    output.memo = memo;
    output.value = 2;

    auto value = output.ToValue();
    EXPECT_EQ(output, OrchardOutput::FromValue(value).value());
  }

  {
    ZCashTransaction::OrchardOutput output;
    output.addr.fill(1);

    output.memo = std::nullopt;
    output.value = 2;

    auto value = output.ToValue();
    EXPECT_EQ(output, OrchardOutput::FromValue(value).value());
  }
}

}  // namespace brave_wallet
