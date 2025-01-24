/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_knapsack_solver.h"

#include <utility>

#include "base/containers/span.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "components/grit/brave_components_strings.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::UnorderedElementsAreArray;

namespace brave_wallet {

class BitcoinKnapsackSolverUnitTest : public testing::Test {
 public:
  BitcoinKnapsackSolverUnitTest() = default;
  ~BitcoinKnapsackSolverUnitTest() override = default;

 protected:
  BitcoinTransaction MakeMockTransaction(uint64_t amount,
                                         uint32_t receive_index = 123) {
    BitcoinTransaction transaction;
    transaction.set_to(
        keyring_
            .GetAddress(
                1, mojom::BitcoinKeyId(kBitcoinReceiveIndex, receive_index))
            ->address_string);
    transaction.set_amount(amount);
    transaction.set_locktime(12345);

    BitcoinTransaction::TxOutput target_output;
    target_output.type = BitcoinTransaction::TxOutputType::kTarget;
    target_output.amount = transaction.amount();
    target_output.address = transaction.to();
    target_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        target_output.address, testnet_);
    EXPECT_FALSE(target_output.script_pubkey.empty());
    transaction.AddOutput(std::move(target_output));

    BitcoinTransaction::TxOutput change_output;
    change_output.type = BitcoinTransaction::TxOutputType::kChange;
    change_output.amount = 0;
    change_output.address =
        keyring_.GetAddress(0, mojom::BitcoinKeyId(kBitcoinChangeIndex, 456))
            ->address_string;
    change_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        change_output.address, testnet_);
    EXPECT_FALSE(change_output.script_pubkey.empty());
    transaction.AddOutput(std::move(change_output));

    return transaction;
  }

  BitcoinTransaction::TxInput MakeMockTxInput(uint64_t amount, uint32_t index) {
    auto address =
        keyring_
            .GetAddress(0, mojom::BitcoinKeyId(kBitcoinReceiveIndex, index))
            ->address_string;

    BitcoinTransaction::TxInput tx_input;
    tx_input.utxo_address = address;
    std::string txid_fake = address + base::NumberToString(amount);
    tx_input.utxo_outpoint.txid =
        crypto::hash::Sha256(base::as_byte_span(txid_fake));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid.back();
    tx_input.utxo_value = amount;

    return tx_input;
  }

  uint64_t send_amount() const { return 10000; }
  double fee_rate() const { return 11.1; }
  double longterm_fee_rate() const { return 3.0; }

  bool testnet_ = false;
  BitcoinHDKeyring keyring_{*bip39::MnemonicToSeed(kMnemonicAbandonAbandon),
                            testnet_};
};

TEST_F(BitcoinKnapsackSolverUnitTest, NoInputs) {
  auto base_tx = MakeMockTransaction(send_amount());

  KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(), {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(BitcoinKnapsackSolverUnitTest, NotEnoughInputsForFee) {
  auto base_tx = MakeMockTransaction(send_amount());

  std::vector<BitcoinTransaction::TxInputGroup> input_groups;
  input_groups.emplace_back();
  input_groups.back().AddInput(MakeMockTxInput(send_amount(), 0));
  KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(), input_groups);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(BitcoinKnapsackSolverUnitTest, NoChangeGenerated) {
  auto base_tx = MakeMockTransaction(send_amount());

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee = ApplyFeeRate(fee_rate(), std::ceil(109.25));
  {
    uint32_t total_input = send_amount() + min_fee;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }

  {
    uint32_t total_input = send_amount() + min_fee - 1;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    // We have a bit less than send amount + fee. Can't create transaction.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
              tx.error());
  }

  {
    uint32_t total_input = send_amount() + min_fee + 1;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have a bit more than send amount + fee. Still no change. Surplus goes
    // to fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee + 1);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }
}

TEST_F(BitcoinKnapsackSolverUnitTest, NoDustChangeGenerated) {
  auto base_tx = MakeMockTransaction(send_amount());

  // Fee for typical 1 input -> 2 outputs transaction.
  uint32_t min_fee = ApplyFeeRate(fee_rate(), std::ceil(140.25));
  EXPECT_EQ(min_fee, 1566u);

  // https://github.com/bitcoin/bitcoin/blob/v25.1/src/policy/policy.cpp#L57
  // Change ouput with less than this amount is not worth creating.
  uint32_t dust_change_threshold =
      ApplyFeeRate(fee_rate(), std::ceil(31.0)) +
      ApplyFeeRate(longterm_fee_rate(), std::ceil(67.75));
  EXPECT_EQ(dust_change_threshold, 549u);

  {
    uint32_t total_input = send_amount() + min_fee + dust_change_threshold;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // Change output is created and has exactly `dust_change_threshold` amount.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount() + dust_change_threshold);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold);
  }

  {
    uint32_t total_input = send_amount() + min_fee + dust_change_threshold - 1;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly less than needed for change output, so it is not created
    // and surplus goes to fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee + dust_change_threshold - 1);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }

  {
    uint32_t total_input = send_amount() + min_fee + dust_change_threshold + 1;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(),
                          input_groups);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly more than needed for change output which all goes to
    // change.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(),
              send_amount() + dust_change_threshold + 1);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold + 1);
  }
}

TEST_F(BitcoinKnapsackSolverUnitTest, GroupIsSpentAsAWhole) {
  auto base_tx = MakeMockTransaction(send_amount());

  std::vector<BitcoinTransaction::TxInputGroup> input_groups;
  input_groups.emplace_back();
  input_groups.back().AddInput(MakeMockTxInput(send_amount(), 0));
  input_groups.back().AddInput(MakeMockTxInput(100000, 0));
  input_groups.back().AddInput(MakeMockTxInput(200000, 0));
  input_groups.back().AddInput(MakeMockTxInput(300000, 1));
  input_groups.back().AddInput(MakeMockTxInput(400000, 1));

  KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(), input_groups);

  // Large portion of inputs is spent to change as inputs group is not allowed
  // to be split.
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
  EXPECT_EQ(tx->amount(), send_amount());

  EXPECT_EQ(tx->EffectiveFeeAmount(), 4574u);

  EXPECT_EQ(tx->TotalInputsAmount(), 1010000u);

  EXPECT_EQ(tx->TotalOutputsAmount(), 1005426u);
  EXPECT_EQ(tx->TargetOutput()->amount, 10000u);
  EXPECT_EQ(tx->ChangeOutput()->amount, 995426u);

  EXPECT_THAT(tx.value().inputs(),
              UnorderedElementsAreArray(
                  {MakeMockTxInput(send_amount(), 0),
                   MakeMockTxInput(100000, 0), MakeMockTxInput(200000, 0),
                   MakeMockTxInput(300000, 1), MakeMockTxInput(400000, 1)}));
}

TEST_F(BitcoinKnapsackSolverUnitTest, RandomTest) {
  std::vector<BitcoinTransaction::TxInputGroup> input_groups;

  uint64_t total_inputs = 0;

  for (int i = 0; i < 100; ++i) {
    if (base::RandInt(0, 10) == 0 || input_groups.empty()) {
      input_groups.emplace_back();
    }
    auto input =
        MakeMockTxInput(base::RandInt(0, 10000000), base::RandInt(0, 10));
    total_inputs += input.utxo_value;
    input_groups.back().AddInput(std::move(input));
  }

  auto base_tx = MakeMockTransaction(total_inputs / 2);

  KnapsackSolver solver(base_tx, fee_rate(), longterm_fee_rate(), input_groups);
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
}

}  // namespace brave_wallet
