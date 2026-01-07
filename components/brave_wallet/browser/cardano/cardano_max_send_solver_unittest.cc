/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_send_solver.h"

#include <utility>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

uint64_t MinFeeForTxSize(uint64_t min_fee_coefficient,
                         uint64_t min_fee_constant,
                         uint32_t tx_size) {
  return tx_size * min_fee_coefficient + min_fee_constant;
}

}  // namespace

class CardanoMaxSendSolverUnitTest : public testing::Test {
 public:
  CardanoMaxSendSolverUnitTest() = default;
  ~CardanoMaxSendSolverUnitTest() override = default;

 protected:
  CardanoTransaction MakeMockTransaction(uint32_t receive_index = 123) {
    CardanoTransaction transaction;
    transaction.set_to(*CardanoAddress::FromString(
        keyring_
            .GetAddress(1, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal,
                                               receive_index))
            ->address_string));
    transaction.set_invalid_after(12345);

    CardanoTransaction::TxOutput target_output;
    target_output.type = CardanoTransaction::TxOutputType::kTarget;
    target_output.amount = transaction.amount();
    target_output.address = transaction.to();
    transaction.AddOutput(std::move(target_output));

    transaction.set_sending_max_amount(true);

    return transaction;
  }

  CardanoTransaction::TxInput MakeMockTxInput(uint64_t amount) {
    auto address = keyring_
                       .GetAddress(0, mojom::CardanoKeyId(
                                          mojom::CardanoKeyRole::kExternal, 0))
                       ->address_string;

    uint32_t id = next_input_id_++;
    CardanoTransaction::TxInput tx_input;
    tx_input.utxo_address = *CardanoAddress::FromString(address);
    tx_input.utxo_outpoint.txid =
        crypto::hash::Sha256(base::byte_span_from_ref(id));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid[0];
    tx_input.utxo_value = amount;

    return tx_input;
  }

  CardanoAddress GetChangeAddress() {
    return *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, 456))
            ->address_string);
  }

  uint64_t send_amount() const { return 1000001; }
  uint64_t min_fee_coefficient() const { return 44; }
  uint64_t min_fee_constant() const { return 155381; }
  uint64_t coins_per_utxo_size() const { return 4310; }

  cardano_rpc::EpochParameters latest_epoch_parameters() {
    return cardano_rpc::EpochParameters{
        .min_fee_coefficient = min_fee_coefficient(),
        .min_fee_constant = min_fee_constant(),
        .coins_per_utxo_size = coins_per_utxo_size()};
  }
  uint32_t dust_change_threshold() const { return 3036; }

  CardanoHDKeyring keyring_{*bip39::MnemonicToSeed(kMnemonicAbandonAbandon),
                            mojom::KeyringId::kCardanoMainnet};

  uint32_t next_input_id_ = 0;
};

TEST_F(CardanoMaxSendSolverUnitTest, NoInputs) {
  auto base_tx = MakeMockTransaction();

  CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                              latest_epoch_parameters(), {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxSendSolverUnitTest, NotEnoughInputsForFee) {
  auto base_tx = MakeMockTransaction();

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(send_amount()));
  CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                              latest_epoch_parameters(), inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxSendSolverUnitTest, NoChangeNeeded) {
  auto base_tx = MakeMockTransaction();

  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
    EXPECT_EQ(min_fee, 165501u);

    uint32_t total_input = send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Sending twice of send_amount.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
    EXPECT_EQ(min_fee, 165501u);

    uint32_t total_input = 2 * send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), 2 * send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, 2 * send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Sending slightly less than send_amount.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
    EXPECT_EQ(min_fee, 165501u);

    uint32_t total_input = send_amount() - 1000 + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), send_amount() - 1000);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount() - 1000);
    EXPECT_FALSE(tx->ChangeOutput());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Sending one tenth of send_amount fails min value req.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
    EXPECT_EQ(min_fee, 165501u);

    uint32_t total_input = send_amount() / 10 + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_FALSE(tx.has_value());

    EXPECT_EQ(tx.error(), WalletInsufficientBalanceErrorMessage());
  }
}

TEST_F(CardanoMaxSendSolverUnitTest, ManyInputs) {
  auto base_tx = MakeMockTransaction();

  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 3886u);
  EXPECT_EQ(min_fee, 326365u);

  {
    uint32_t total_input = 100 * send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(send_amount() + min_fee));
    for (int i = 1; i < 100; ++i) {
      inputs.push_back(MakeMockTxInput(send_amount()));
    }
    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(uint64_t(tx->GetTotalInputsAmount().ValueOrDie()), total_input);
    EXPECT_EQ(tx->inputs().size(), 100u);
    EXPECT_EQ(uint64_t(tx->GetTotalOutputsAmount().ValueOrDie()),
              100 * send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, 100 * send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

TEST_F(CardanoMaxSendSolverUnitTest, TokensGoToChange) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  std::vector<CardanoTransaction::TxInput> inputs;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 1'000'000'000;
  inputs.back().utxo_tokens[bar_token] = 1'000'000'000;
  inputs.back().utxo_tokens[baz_token] = 1'000'000'000;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 10;
  inputs.back().utxo_tokens[bar_token] = 100'000'000;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 1;

  inputs.push_back(MakeMockTxInput(1'000'000));

  {
    uint32_t total_input = 1'000'000 * 4;

    uint32_t min_ada_for_change = (156 + 160) * coins_per_utxo_size();

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 497u);
    EXPECT_EQ(fee, 177249u);

    auto base_tx = MakeMockTransaction();

    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->inputs().size(), 4u);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount,
              total_input - fee - min_ada_for_change);
    EXPECT_TRUE(tx->TargetOutput()->tokens.empty());

    // Change output has minimum ADA amount for given utxo.
    EXPECT_EQ(tx->ChangeOutput()->amount, min_ada_for_change);
    // All tokens appear in change output.
    EXPECT_EQ(tx->ChangeOutput()->tokens[foo_token], 1'000'000'011u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[bar_token], 1'100'000'000u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[baz_token], 1'000'000'000u);

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Remove input with no tokens.
  inputs.pop_back();

  {
    uint32_t total_input = 1'000'000 * 3;

    uint32_t min_ada_for_change = (156 + 160) * coins_per_utxo_size();

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 460u);
    EXPECT_EQ(fee, 175621u);

    auto base_tx = MakeMockTransaction();

    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount,
              total_input - fee - min_ada_for_change);
    EXPECT_TRUE(tx->TargetOutput()->tokens.empty());

    // Change output has minimum ADA amount for given utxo.
    EXPECT_EQ(tx->ChangeOutput()->amount, min_ada_for_change);
    // All tokens appear in change output.
    EXPECT_EQ(tx->ChangeOutput()->tokens[foo_token], 1'000'000'011u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[bar_token], 1'100'000'000u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[baz_token], 1'000'000'000u);

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Clear tokens from the first input.
  inputs.front().utxo_tokens.clear();

  {
    uint32_t total_input = 1'000'000 * 3;

    uint32_t min_ada_for_change = (143 + 160) * coins_per_utxo_size();

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 447u);
    EXPECT_EQ(fee, 175049u);

    auto base_tx = MakeMockTransaction();

    CardanoMaxSendSolver solver(base_tx, GetChangeAddress(),
                                latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount,
              total_input - fee - min_ada_for_change);
    EXPECT_TRUE(tx->TargetOutput()->tokens.empty());

    // Change output has minimum ADA amount for given utxo.
    EXPECT_EQ(tx->ChangeOutput()->amount, min_ada_for_change);
    // All tokens appear in change output.
    EXPECT_EQ(tx->ChangeOutput()->tokens[foo_token], 11u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[bar_token], 100'000'000u);
    EXPECT_FALSE(tx->ChangeOutput()->tokens.contains(baz_token));

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

}  // namespace brave_wallet
