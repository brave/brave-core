/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <utility>

#include "base/containers/span.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::UnorderedElementsAreArray;

namespace brave_wallet {

namespace {

uint64_t ApplyFeeRate(uint64_t min_fee_coefficient,
                      uint64_t min_fee_constant,
                      uint32_t tx_size) {
  return tx_size * min_fee_coefficient + min_fee_constant;
}

}  // namespace

class CardanoKnapsackSolverUnitTest : public testing::Test {
 public:
  CardanoKnapsackSolverUnitTest() = default;
  ~CardanoKnapsackSolverUnitTest() override = default;

 protected:
  CardanoTransaction MakeMockTransaction(uint64_t amount,
                                         uint32_t receive_index = 123) {
    CardanoTransaction transaction;
    transaction.set_to(*CardanoAddress::FromString(
        keyring_
            .GetAddress(1, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal,
                                               receive_index))
            ->address_string));
    transaction.set_amount(amount);
    transaction.set_invalid_after(12345);

    CardanoTransaction::TxOutput target_output;
    target_output.type = CardanoTransaction::TxOutputType::kTarget;
    target_output.amount = transaction.amount();
    target_output.address = transaction.to();
    transaction.AddOutput(std::move(target_output));

    CardanoTransaction::TxOutput change_output;
    change_output.type = CardanoTransaction::TxOutputType::kChange;
    change_output.amount = 0;
    change_output.address = *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, 456))
            ->address_string);
    transaction.AddOutput(std::move(change_output));

    return transaction;
  }

  CardanoTransaction::TxInput MakeMockTxInput(uint64_t amount, uint32_t index) {
    auto address =
        keyring_
            .GetAddress(
                0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, index))
            ->address_string;

    CardanoTransaction::TxInput tx_input;
    tx_input.utxo_address = *CardanoAddress::FromString(address);
    std::string txid_fake = address + base::NumberToString(amount);
    tx_input.utxo_outpoint.txid =
        crypto::hash::Sha256(base::as_byte_span(txid_fake));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid.back();
    tx_input.utxo_value = amount;

    return tx_input;
  }

  uint64_t send_amount() const { return 10000; }
  uint64_t min_fee_coefficient() const { return 44; }
  uint64_t min_fee_constant() const { return 155381; }
  cardano_rpc::EpochParameters latest_epoch_parameters() {
    return cardano_rpc::EpochParameters{
        .min_fee_coefficient = min_fee_coefficient(),
        .min_fee_constant = min_fee_constant()};
  }
  uint32_t dust_change_threshold() const { return 2684; }

  bool testnet_ = false;
  CardanoHDKeyring keyring_{*bip39::MnemonicToSeed(kMnemonicAbandonAbandon),
                            mojom::KeyringId::kCardanoMainnet};
};

TEST_F(CardanoKnapsackSolverUnitTest, NoInputs) {
  auto base_tx = MakeMockTransaction(send_amount());

  CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NotEnoughInputsForFee) {
  auto base_tx = MakeMockTransaction(send_amount());

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(send_amount(), 0));
  CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NoChangeGenerated) {
  auto base_tx = MakeMockTransaction(send_amount());

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee =
      ApplyFeeRate(min_fee_coefficient(), min_fee_constant(), 222u);
  EXPECT_EQ(min_fee, 165149u);

  {
    uint32_t total_input = send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
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
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    // We have a bit less than send amount + fee. Can't create transaction.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
              tx.error());
  }

  {
    uint32_t total_input = send_amount() + min_fee + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
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

TEST_F(CardanoKnapsackSolverUnitTest, NoDustChangeGenerated) {
  auto base_tx = MakeMockTransaction(send_amount());

  // Fee for typical 1 input -> 2 outputs transaction.
  uint32_t min_fee =
      ApplyFeeRate(min_fee_coefficient(), min_fee_constant(), 283u);
  EXPECT_EQ(min_fee, 167833u);

  {
    uint32_t total_input = send_amount() + min_fee + dust_change_threshold();
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // Change output is created and has exactly `dust_change_threshold` amount.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(),
              send_amount() + dust_change_threshold());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold());
  }

  {
    uint32_t total_input =
        send_amount() + min_fee + dust_change_threshold() - 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly less than needed for change output, so it is not created
    // and surplus goes to fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee + dust_change_threshold() - 1);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }

  {
    uint32_t total_input =
        send_amount() + min_fee + dust_change_threshold() + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly more than needed for change output which all goes to
    // change.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(),
              send_amount() + dust_change_threshold() + 1);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold() + 1);
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, RandomTest) {
  std::vector<CardanoTransaction::TxInput> inputs;

  uint64_t total_inputs = 0;

  for (int i = 0; i < 100; ++i) {
    auto input =
        MakeMockTxInput(base::RandInt(1, 10000000), base::RandInt(0, 10));
    total_inputs += input.utxo_value;
    inputs.push_back(std::move(input));
  }

  auto base_tx = MakeMockTransaction(total_inputs / 2);

  CardanoKnapsackSolver solver(base_tx, latest_epoch_parameters(), inputs);
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
}

}  // namespace brave_wallet
