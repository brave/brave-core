/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

#include <array>
#include <limits>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using testing::_;

namespace brave_wallet {
namespace {
constexpr char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
constexpr char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
constexpr char kAddress1[] = "t1WU75sSfiPbK5ez33uuhEbd9ZD3XNCxMRj";
constexpr char kAddress2[] = "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS";

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
  tx.init_v5_part();

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
  output1.script_pubkey = ZCashAddressToScriptPubkey(kAddress1, false).value();
  output1.amount = 5;
  tx.transparent_part().outputs.push_back(std::move(output1));

  ZCashTransaction::TxOutput output2;
  output2.address = kAddress2;
  output2.script_pubkey = ZCashAddressToScriptPubkey(kAddress2, false).value();
  output2.amount = 50;
  tx.transparent_part().outputs.push_back(std::move(output2));

  OrchardMemo memo;
  memo.fill(2);

  ZCashTransaction::OrchardOutput orchard_output;
  orchard_output.value = 100;
  orchard_output.addr.fill(2);
  orchard_output.memo = memo;
  tx.v5_part().orchard.outputs.push_back(std::move(orchard_output));

  tx.set_to(kAddress1);
  tx.set_amount(12345);
  tx.set_locktime(777);
  tx.set_memo(memo);

  auto parsed = tx.FromValue(tx.ToValue());
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed, tx);
  EXPECT_EQ(parsed->transparent_part().inputs, tx.transparent_part().inputs);
  EXPECT_EQ(parsed->transparent_part().outputs, tx.transparent_part().outputs);
  EXPECT_EQ(parsed->v5_part().orchard.outputs, tx.v5_part().orchard.outputs);
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
  tx.init_v5_part();
  EXPECT_EQ(tx.TotalInputsAmount().ValueOrDie(), 0u);

  ZCashTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  tx.transparent_part().inputs.push_back(std::move(input1));
  EXPECT_EQ(tx.TotalInputsAmount().ValueOrDie(), 555666777u);

  ZCashTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  tx.transparent_part().inputs.push_back(input2);
  EXPECT_EQ(tx.TotalInputsAmount().ValueOrDie(), 555666777u + 555u);
}

TEST(ZCashTransaction, TotalInputsAmountOverflow) {
  ZCashTransaction tx;
  tx.init_v5_part();
  tx.transparent_part().inputs.emplace_back().utxo_value =
      std::numeric_limits<uint64_t>::max();
  tx.v5_part().orchard.inputs.emplace_back().note.amount = 1;

  EXPECT_FALSE(tx.TotalInputsAmount().IsValid());
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
TEST(ZCashTransactionUtilsUnitTest, ValidateAmounts) {
  // Valid transparent-only transaction
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    // Add transparent inputs
    auto& input1 = tx.transparent_part().inputs.emplace_back();
    input1.utxo_value = 10000u;
    auto& input2 = tx.transparent_part().inputs.emplace_back();
    input2.utxo_value = 20000u;

    // Add transparent outputs
    auto& output1 = tx.transparent_part().outputs.emplace_back();
    output1.amount = 15000u;
    auto& output2 = tx.transparent_part().outputs.emplace_back();
    output2.amount = 10000u;

    // 30000 (inputs) = 25000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Valid transparent-only transaction with single input/output
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 10000u;

    auto& output = tx.transparent_part().outputs.emplace_back();
    output.amount = 5000u;

    // 10000 (input) = 5000 (output) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Invalid transparent transaction - inputs < outputs + fee
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 10000u;

    auto& output = tx.transparent_part().outputs.emplace_back();
    output.amount = 6000u;

    // 10000 (input) < 6000 (output) + 5000 (fee) = 11000
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  // Invalid transparent transaction - inputs > outputs + fee
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 20000u;

    auto& output = tx.transparent_part().outputs.emplace_back();
    output.amount = 10000u;

    // 20000 (input) > 10000 (output) + 5000 (fee) = 15000
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  // Valid transaction with zero fee
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(0u);

    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 10000u;

    auto& output = tx.transparent_part().outputs.emplace_back();
    output.amount = 10000u;

    // 10000 (input) = 10000 (output) + 0 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Valid transaction with empty inputs and outputs (zero fee)
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(0u);

    // 0 (inputs) = 0 (outputs) + 0 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Test with multiple transparent inputs and outputs
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    // Multiple inputs
    for (uint64_t value : {5000u, 10000u, 15000u, 20000u}) {
      auto& input = tx.transparent_part().inputs.emplace_back();
      input.utxo_value = value;
    }

    // Multiple outputs
    for (uint64_t amount : {10000u, 15000u, 20000u}) {
      auto& output = tx.transparent_part().outputs.emplace_back();
      output.amount = amount;
    }

    // 50000 (inputs) = 45000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Valid orchard-only transaction
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    // Add orchard inputs
    auto& input1 = tx.v5_part().orchard.inputs.emplace_back();
    input1.note.amount = 10000u;
    auto& input2 = tx.v5_part().orchard.inputs.emplace_back();
    input2.note.amount = 20000u;

    // Add orchard outputs
    auto& output1 = tx.v5_part().orchard.outputs.emplace_back();
    output1.value = 15000u;
    auto& output2 = tx.v5_part().orchard.outputs.emplace_back();
    output2.value = 10000u;

    // 30000 (inputs) = 25000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Valid mixed transaction (transparent + orchard)
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    // Transparent inputs
    auto& t_input = tx.transparent_part().inputs.emplace_back();
    t_input.utxo_value = 10000u;

    // Orchard inputs
    auto& o_input = tx.v5_part().orchard.inputs.emplace_back();
    o_input.note.amount = 20000u;

    // Transparent outputs
    auto& t_output = tx.transparent_part().outputs.emplace_back();
    t_output.amount = 15000u;

    // Orchard outputs
    auto& o_output = tx.v5_part().orchard.outputs.emplace_back();
    o_output.value = 10000u;

    // 30000 (inputs) = 25000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Invalid mixed transaction - inputs < outputs + fee
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& t_input = tx.transparent_part().inputs.emplace_back();
    t_input.utxo_value = 10000u;

    auto& o_input = tx.v5_part().orchard.inputs.emplace_back();
    o_input.note.amount = 5000u;

    auto& t_output = tx.transparent_part().outputs.emplace_back();
    t_output.amount = 10000u;

    auto& o_output = tx.v5_part().orchard.outputs.emplace_back();
    o_output.value = 6000u;

    // 15000 (inputs) < 16000 (outputs) + 5000 (fee) = 21000
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  // Invalid mixed transaction - inputs > outputs + fee
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& t_input = tx.transparent_part().inputs.emplace_back();
    t_input.utxo_value = 20000u;

    auto& o_input = tx.v5_part().orchard.inputs.emplace_back();
    o_input.note.amount = 10000u;

    auto& t_output = tx.transparent_part().outputs.emplace_back();
    t_output.amount = 10000u;

    auto& o_output = tx.v5_part().orchard.outputs.emplace_back();
    o_output.value = 5000u;

    // 30000 (inputs) > 15000 (outputs) + 5000 (fee) = 20000
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  // Valid transaction with multiple orchard inputs and outputs
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(10000u);

    // Multiple orchard inputs
    for (uint64_t amount : {10000u, 20000u, 30000u}) {
      auto& input = tx.v5_part().orchard.inputs.emplace_back();
      input.note.amount = amount;
    }

    // Multiple orchard outputs (total = 50000)
    for (uint64_t value : {15000u, 20000u, 15000u}) {
      auto& output = tx.v5_part().orchard.outputs.emplace_back();
      output.value = value;
    }

    // 60000 (inputs) = 50000 (outputs) + 10000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Valid transaction with orchard inputs and transparent output
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(5000u);

    auto& input1 = tx.v5_part().orchard.inputs.emplace_back();
    input1.note.amount = 10000u;
    auto& input2 = tx.v5_part().orchard.inputs.emplace_back();
    input2.note.amount = 20000u;

    // Transparent output (orchard to transparent transaction)
    auto& t_output = tx.transparent_part().outputs.emplace_back();
    t_output.amount = 25000u;

    // 30000 (inputs) = 25000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Test with large amounts
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_fee(1000u);

    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 10000000000u;

    auto& output = tx.transparent_part().outputs.emplace_back();
    output.amount = 9999999000u;

    // 10000000000 (input) = 9999999000 (output) + 1000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }
}

TEST(ZCashTransactionUtilsUnitTest, ValidateAmountsOverflow) {
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.transparent_part().inputs.emplace_back().utxo_value =
        std::numeric_limits<uint64_t>::max();
    tx.v5_part().orchard.inputs.emplace_back().note.amount = 1;
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.transparent_part().inputs.emplace_back().utxo_value =
        std::numeric_limits<uint64_t>::max();
    tx.transparent_part().outputs.emplace_back().amount =
        std::numeric_limits<uint64_t>::max();
    tx.v5_part().orchard.outputs.emplace_back().value = 1;
    EXPECT_FALSE(tx.ValidateAmounts());
  }

  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.transparent_part().inputs.emplace_back().utxo_value =
        std::numeric_limits<uint64_t>::max();
    tx.transparent_part().outputs.emplace_back().amount =
        std::numeric_limits<uint64_t>::max();
    tx.set_fee(1);
    EXPECT_FALSE(tx.ValidateAmounts());
  }
}

// Regression tests for ZCashTransaction::operator==.
// Guards the fix where the second tuple entry incorrectly compared
// this->orchard_part_ against itself instead of other.orchard_part_.
TEST(ZCashTransaction, OperatorEquals_OrchardPart) {
  ZCashTransaction tx1;
  ZCashTransaction tx2;
  tx1.init_v5_part();
  tx2.init_v5_part();

  // Identical transactions must be equal.
  EXPECT_EQ(tx1, tx2);

  // A difference in orchard_part must be detected.
  tx1.v5_part().orchard.outputs.emplace_back();  // add one output
  EXPECT_NE(tx1, tx2);

  // After making tx2 match tx1 they must be equal again.
  tx2.v5_part().orchard.outputs.emplace_back();
  EXPECT_EQ(tx1, tx2);
}

TEST(ZCashTransaction, ToValueFromValue_VersionRoundTrip) {
  // v5 round-trips to a v5 transaction.
  {
    ZCashTransaction tx;
    tx.init_v5_part();
    tx.set_locktime(10);
    tx.set_to("t1example");
    auto& input = tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 5;
    auto parsed = ZCashTransaction::FromValue(tx.ToValue());
    ASSERT_TRUE(parsed);
    EXPECT_TRUE(parsed->is_v5());
    EXPECT_EQ(*parsed, tx);
  }
  // v6 round-trips to a v6 transaction.
  {
    ZCashTransaction tx;
    tx.init_v6_part();
    tx.set_locktime(11);
    tx.set_to("t1example");
    auto parsed = ZCashTransaction::FromValue(tx.ToValue());
    ASSERT_TRUE(parsed);
    EXPECT_TRUE(parsed->is_v6());
    EXPECT_EQ(*parsed, tx);
  }
}

TEST(ZCashTransaction, OperatorEquals_V6Part) {
  ZCashTransaction tx1;
  ZCashTransaction tx2;

  // Both uninitialized: equal.
  EXPECT_EQ(tx1, tx2);

  // Only tx1 has v6_part: not equal.
  tx1.init_v6_part();
  EXPECT_NE(tx1, tx2);

  // Both have default v6_part: equal.
  tx2.init_v6_part();
  EXPECT_EQ(tx1, tx2);

  // Differing legacy_orchard digest: not equal.
  std::array<uint8_t, kZCashDigestSize> d{};
  d[0] = 0xAB;
  tx1.v6_part().legacy_orchard.digest = d;
  EXPECT_NE(tx1, tx2);

  tx2.v6_part().legacy_orchard.digest = d;
  EXPECT_EQ(tx1, tx2);

  // Differing ironwood digest: not equal.
  std::array<uint8_t, kZCashDigestSize> d2{};
  d2[0] = 0xCD;
  tx1.v6_part().ironwood.digest = d2;
  EXPECT_NE(tx1, tx2);

  tx2.v6_part().ironwood.digest = d2;
  EXPECT_EQ(tx1, tx2);
}

TEST(ZCashTransaction, LegacyFormatReadsAsV5AndWritesV5Part) {
  // Frozen pre-v6 persisted shape: top-level orchard_* keys, no v6_part.
  // note_version is omitted so OrchardNote::FromValue defaults it to 2.
  constexpr char kOrchardAddrHex[] =
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAA";  // 43 bytes
  constexpr char kNullifierHex[] =
      "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";  // 32
  constexpr char kRhoHex[] =
      "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";  // 32
  constexpr char kSeedHex[] =
      "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD";  // 32
  constexpr char kOutputAddrHex[] =
      "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
      "EEEEEEEEEEEEEEEE";  // 43 bytes

  base::DictValue dict = base::test::ParseJsonDict(absl::StrFormat(
      R"({
        "inputs": [{
          "utxo_address": "%s",
          "utxo_outpoint": {
            "txid": "%s",
            "index": 1
          },
          "utxo_value": "100000",
          "script_pub_key": "010203",
          "script_sig": "0405"
        }],
        "outputs": [{
          "address": "%s",
          "amount": "40000",
          "script_pub_key": "060708"
        }],
        "orchard_inputs": [{
          "note": {
            "addr": "%s",
            "block_id": "42",
            "nullifier": "%s",
            "amount": "50000",
            "orchard_commitment_tree_position": "7",
            "rho": "%s",
            "seed": "%s"
          }
        }],
        "orchard_outputs": [{
          "address": "%s",
          "amount": "45000"
        }],
        "anchor_block_height": "1234",
        "locktime": "99",
        "to": "%s",
        "amount": "45000",
        "fee": "15000",
        "expiry_height": "2000"
      })",
      kAddress1, kTxid1, kAddress2, kOrchardAddrHex, kNullifierHex, kRhoHex,
      kSeedHex, kOutputAddrHex, kAddress2));

  auto parsed = ZCashTransaction::FromValue(dict);
  ASSERT_TRUE(parsed);
  EXPECT_TRUE(parsed->is_v5());
  EXPECT_FALSE(parsed->is_v6());

  EXPECT_EQ(parsed->locktime(), 99u);
  EXPECT_EQ(parsed->to(), kAddress2);
  EXPECT_EQ(parsed->amount(), 45000u);
  EXPECT_EQ(parsed->fee(), 15000u);
  EXPECT_EQ(parsed->expiry_height(), 2000u);

  ASSERT_EQ(parsed->transparent_part().inputs.size(), 1u);
  EXPECT_EQ(parsed->transparent_part().inputs[0].utxo_address, kAddress1);
  EXPECT_EQ(parsed->transparent_part().inputs[0].utxo_value, 100000u);

  ASSERT_EQ(parsed->transparent_part().outputs.size(), 1u);
  EXPECT_EQ(parsed->transparent_part().outputs[0].address, kAddress2);
  EXPECT_EQ(parsed->transparent_part().outputs[0].amount, 40000u);

  ASSERT_EQ(parsed->v5_part().orchard.inputs.size(), 1u);
  EXPECT_EQ(parsed->v5_part().orchard.inputs[0].note.amount, 50000u);
  EXPECT_EQ(parsed->v5_part().orchard.inputs[0].note.block_id, 42u);
  EXPECT_EQ(parsed->v5_part().orchard.inputs[0].note.note_version, 2u);
  EXPECT_EQ(
      parsed->v5_part().orchard.inputs[0].note.orchard_commitment_tree_position,
      7u);

  ASSERT_EQ(parsed->v5_part().orchard.outputs.size(), 1u);
  EXPECT_EQ(parsed->v5_part().orchard.outputs[0].value, 45000u);
  ASSERT_TRUE(parsed->v5_part().orchard.anchor_block_height);
  EXPECT_EQ(*parsed->v5_part().orchard.anchor_block_height, 1234u);

  // Re-serializing migrates the legacy top-level shape to explicit v5 data.
  base::DictValue rewritten = parsed->ToValue();
  EXPECT_FALSE(rewritten.FindDict("v6_part"));
  auto* v5_part = rewritten.FindDict("v5_part");
  ASSERT_TRUE(v5_part);
  auto* orchard = v5_part->FindDict("orchard");
  ASSERT_TRUE(orchard);
  ASSERT_TRUE(orchard->FindList("inputs"));
  ASSERT_TRUE(orchard->FindList("outputs"));
  EXPECT_TRUE(orchard->FindString("anchor_block_height"));
  EXPECT_FALSE(rewritten.FindList("orchard_inputs"));
  EXPECT_FALSE(rewritten.FindList("orchard_outputs"));
}

TEST(ZCashTransaction, FromValue_OldFormatOrchardOnlyParsesAsV5) {
  constexpr char kOrchardAddrHex[] =
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAA";
  constexpr char kNullifierHex[] =
      "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
  constexpr char kRhoHex[] =
      "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";
  constexpr char kSeedHex[] =
      "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD";
  constexpr char kOutputAddrHex[] =
      "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
      "EEEEEEEEEEEEEEEE";

  // No transparent inputs/outputs lists — only orchard_* (accepted pre-v6
  // path).
  base::DictValue dict = base::test::ParseJsonDict(absl::StrFormat(
      R"({
        "orchard_inputs": [{
          "note": {
            "addr": "%s",
            "block_id": "1",
            "nullifier": "%s",
            "amount": "30000",
            "orchard_commitment_tree_position": "0",
            "rho": "%s",
            "seed": "%s"
          }
        }],
        "orchard_outputs": [{
          "address": "%s",
          "amount": "25000"
        }],
        "anchor_block_height": "10",
        "locktime": "0",
        "to": "ua1example",
        "amount": "25000",
        "fee": "5000"
      })",
      kOrchardAddrHex, kNullifierHex, kRhoHex, kSeedHex, kOutputAddrHex));

  auto parsed = ZCashTransaction::FromValue(dict);
  ASSERT_TRUE(parsed);
  EXPECT_TRUE(parsed->is_v5());
  EXPECT_TRUE(parsed->transparent_part().inputs.empty());
  EXPECT_TRUE(parsed->transparent_part().outputs.empty());
  ASSERT_EQ(parsed->v5_part().orchard.inputs.size(), 1u);
  EXPECT_EQ(parsed->v5_part().orchard.inputs[0].note.amount, 30000u);
  ASSERT_EQ(parsed->v5_part().orchard.outputs.size(), 1u);
  EXPECT_EQ(parsed->v5_part().orchard.outputs[0].value, 25000u);
  EXPECT_EQ(*parsed->v5_part().orchard.anchor_block_height, 10u);
}

TEST(ZCashTransaction, FromValue_LegacyTransparentOnlyParsesAsV5) {
  base::DictValue dict = base::test::ParseJsonDict(absl::StrFormat(
      R"({
        "inputs": [{
          "utxo_address": "%s",
          "utxo_outpoint": {
            "txid": "%s",
            "index": 1
          },
          "utxo_value": "100000",
          "script_pub_key": "010203",
          "script_sig": "0405"
        }],
        "outputs": [{
          "address": "%s",
          "amount": "90000",
          "script_pub_key": "060708"
        }],
        "orchard_inputs": [],
        "orchard_outputs": [],
        "locktime": "99",
        "to": "%s",
        "amount": "90000",
        "fee": "10000"
      })",
      kAddress1, kTxid1, kAddress2, kAddress2));

  auto parsed = ZCashTransaction::FromValue(dict);
  ASSERT_TRUE(parsed);
  EXPECT_TRUE(parsed->is_v5());
  EXPECT_FALSE(parsed->is_v6());
  EXPECT_EQ(parsed->locktime(), 99u);
  EXPECT_EQ(parsed->to(), kAddress2);
  EXPECT_EQ(parsed->amount(), 90000u);
  EXPECT_EQ(parsed->fee(), 10000u);

  ASSERT_EQ(parsed->transparent_part().inputs.size(), 1u);
  EXPECT_EQ(parsed->transparent_part().inputs[0].utxo_address, kAddress1);
  EXPECT_EQ(parsed->transparent_part().inputs[0].utxo_value, 100000u);
  ASSERT_EQ(parsed->transparent_part().outputs.size(), 1u);
  EXPECT_EQ(parsed->transparent_part().outputs[0].address, kAddress2);
  EXPECT_EQ(parsed->transparent_part().outputs[0].amount, 90000u);

  EXPECT_TRUE(parsed->v5_part().orchard.inputs.empty());
  EXPECT_TRUE(parsed->v5_part().orchard.outputs.empty());
}

TEST(ZCashTransaction, InitVersionPart) {
  ZCashTransaction tx;
  EXPECT_FALSE(tx.is_v5());
  EXPECT_FALSE(tx.is_v6());

  tx.init_v5_part();
  EXPECT_TRUE(tx.is_v5());
  EXPECT_FALSE(tx.is_v6());

  tx.init_v6_part();
  EXPECT_FALSE(tx.is_v5());
  EXPECT_TRUE(tx.is_v6());
  EXPECT_TRUE(tx.v6_part().legacy_orchard.inputs.empty());
  EXPECT_TRUE(tx.v6_part().legacy_orchard.outputs.empty());
  EXPECT_TRUE(tx.v6_part().ironwood.inputs.empty());
  EXPECT_TRUE(tx.v6_part().ironwood.outputs.empty());
}

TEST(ZCashTransaction, ToValueFromValue_V6PartRichRoundTrip) {
  ZCashTransaction tx;
  tx.init_v6_part();
  tx.set_locktime(42);
  tx.set_to(kAddress1);
  tx.set_amount(90000);
  tx.set_fee(10000);
  tx.set_expiry_height(500);

  {
    auto& input = tx.v6_part().legacy_orchard.inputs.emplace_back();
    input.note.addr.fill(0x11);
    input.note.block_id = 3;
    input.note.nullifier.fill(0x22);
    input.note.amount = 70000;
    input.note.orchard_commitment_tree_position = 1;
    input.note.rho.fill(0x33);
    input.note.seed.fill(0x44);
    input.note.note_version = 2;

    auto& output = tx.v6_part().legacy_orchard.outputs.emplace_back();
    output.addr.fill(0x55);
    output.value = 20000;
    tx.v6_part().legacy_orchard.anchor_block_height = 100;
    std::array<uint8_t, kZCashDigestSize> digest{};
    digest.fill(0xAB);
    tx.v6_part().legacy_orchard.digest = digest;
    tx.v6_part().legacy_orchard.raw_tx = std::vector<uint8_t>{0x01, 0x02, 0x03};
  }

  {
    auto& input = tx.v6_part().ironwood.inputs.emplace_back();
    input.note.addr.fill(0x66);
    input.note.block_id = 4;
    input.note.nullifier.fill(0x77);
    input.note.amount = 30000;
    input.note.orchard_commitment_tree_position = 2;
    input.note.rho.fill(0x88);
    input.note.seed.fill(0x99);
    input.note.note_version = 3;

    auto& output = tx.v6_part().ironwood.outputs.emplace_back();
    output.addr.fill(0xAA);
    output.value = 70000;
    tx.v6_part().ironwood.anchor_block_height = 200;
    std::array<uint8_t, kZCashDigestSize> digest{};
    digest.fill(0xCD);
    tx.v6_part().ironwood.digest = digest;
    tx.v6_part().ironwood.raw_tx = std::vector<uint8_t>{0xDE, 0xAD};
  }

  auto& t_input = tx.transparent_part().inputs.emplace_back();
  t_input.utxo_address = kAddress1;
  t_input.utxo_value = 5000;
  base::HexStringToSpan(kTxid1, t_input.utxo_outpoint.txid);

  base::DictValue value = tx.ToValue();
  EXPECT_FALSE(value.FindDict("v5_part"));
  auto* v6_part = value.FindDict("v6_part");
  ASSERT_TRUE(v6_part);
  EXPECT_FALSE(v6_part->Find("zip233_amount"));
  const auto* legacy_orchard = v6_part->FindDict("legacy_orchard");
  ASSERT_TRUE(legacy_orchard);
  EXPECT_FALSE(legacy_orchard->Find("digest"));
  EXPECT_FALSE(legacy_orchard->Find("raw_tx"));
  const auto* ironwood = v6_part->FindDict("ironwood");
  ASSERT_TRUE(ironwood);
  EXPECT_FALSE(ironwood->Find("digest"));
  EXPECT_FALSE(ironwood->Find("raw_tx"));

  auto parsed = ZCashTransaction::FromValue(value);
  ASSERT_TRUE(parsed);
  EXPECT_TRUE(parsed->is_v6());
  ZCashTransaction expected = tx;
  expected.v6_part().legacy_orchard.digest.reset();
  expected.v6_part().legacy_orchard.raw_tx.reset();
  expected.v6_part().ironwood.digest.reset();
  expected.v6_part().ironwood.raw_tx.reset();
  EXPECT_EQ(*parsed, expected);

  base::DictValue rewritten = parsed->ToValue();
  ASSERT_TRUE(rewritten.FindDict("v6_part"));
  EXPECT_FALSE(rewritten.FindList("orchard_inputs"));
  EXPECT_FALSE(rewritten.FindList("orchard_outputs"));
}

TEST(ZCashTransaction, TotalInputsAmount_V6) {
  ZCashTransaction tx;
  tx.init_v6_part();

  EXPECT_EQ(tx.TotalInputsAmount().ValueOrDie(), 0u);

  tx.transparent_part().inputs.emplace_back().utxo_value = 1000u;
  tx.v6_part().legacy_orchard.inputs.emplace_back().note.amount = 2000u;
  tx.v6_part().ironwood.inputs.emplace_back().note.amount = 3000u;
  EXPECT_EQ(tx.TotalInputsAmount().ValueOrDie(), 6000u);
}

TEST(ZCashTransaction, ValidateAmounts_V6) {
  // Valid dual-pool v6 transaction.
  {
    ZCashTransaction tx;
    tx.init_v6_part();
    tx.set_fee(5000u);

    tx.v6_part().legacy_orchard.inputs.emplace_back().note.amount = 20000u;
    tx.v6_part().ironwood.inputs.emplace_back().note.amount = 10000u;
    tx.transparent_part().inputs.emplace_back().utxo_value = 5000u;

    tx.v6_part().legacy_orchard.outputs.emplace_back().value = 10000u;
    tx.v6_part().ironwood.outputs.emplace_back().value = 15000u;
    tx.transparent_part().outputs.emplace_back().amount = 5000u;

    // 35000 (inputs) = 30000 (outputs) + 5000 (fee)
    EXPECT_TRUE(tx.ValidateAmounts());
  }

  // Invalid: inputs < outputs + fee.
  {
    ZCashTransaction tx;
    tx.init_v6_part();
    tx.set_fee(5000u);

    tx.v6_part().legacy_orchard.inputs.emplace_back().note.amount = 10000u;
    tx.v6_part().ironwood.outputs.emplace_back().value = 10000u;

    EXPECT_FALSE(tx.ValidateAmounts());
  }

  // Invalid: inputs > outputs + fee.
  {
    ZCashTransaction tx;
    tx.init_v6_part();
    tx.set_fee(1000u);

    tx.v6_part().ironwood.inputs.emplace_back().note.amount = 20000u;
    tx.v6_part().ironwood.outputs.emplace_back().value = 10000u;

    EXPECT_FALSE(tx.ValidateAmounts());
  }
}

TEST(ZCashTransaction, FromValue_V5PartMalformed) {
  // Missing orchard.
  base::DictValue dict = base::test::ParseJsonDict(R"({
    "v5_part": {},
    "locktime": "0",
    "to": "t1",
    "amount": "0",
    "fee": "0"
  })");
  EXPECT_FALSE(ZCashTransaction::FromValue(dict));
}

TEST(ZCashTransaction, FromValue_V6PartMalformed) {
  // Missing legacy_orchard.
  {
    base::DictValue dict = base::test::ParseJsonDict(R"({
      "v6_part": {
        "ironwood": {}
      },
      "locktime": "0",
      "to": "t1",
      "amount": "0",
      "fee": "0"
    })");
    EXPECT_FALSE(ZCashTransaction::FromValue(dict));
  }

  // Missing ironwood.
  {
    base::DictValue dict = base::test::ParseJsonDict(R"({
      "v6_part": {
        "legacy_orchard": {}
      },
      "locktime": "0",
      "to": "t1",
      "amount": "0",
      "fee": "0"
    })");
    EXPECT_FALSE(ZCashTransaction::FromValue(dict));
  }
}

TEST(ZCashTransaction, FromValue_EmptyDictFails) {
  base::DictValue dict = base::test::ParseJsonDict(R"({
    "locktime": "0",
    "to": "t1",
    "amount": "0",
    "fee": "0"
  })");
  EXPECT_FALSE(ZCashTransaction::FromValue(dict));
}

}  // namespace brave_wallet
