/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_send_solver.h"

#include <utility>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
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
};

TEST_F(CardanoMaxSendSolverUnitTest, NoInputs) {
  auto base_tx = MakeMockTransaction();

  CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxSendSolverUnitTest, NotEnoughInputsForFee) {
  auto base_tx = MakeMockTransaction();

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(send_amount(), 0));
  CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxSendSolverUnitTest, NoChangeNeeded) {
  auto base_tx = MakeMockTransaction();

  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 228u);
    EXPECT_EQ(min_fee, 165413u);

    uint32_t total_input = send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }

  // Sending twice of send_amount.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 228u);
    EXPECT_EQ(min_fee, 165413u);

    uint32_t total_input = 2 * send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), 2 * send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, 2 * send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }

  // Sending slightly less than send_amount.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 228u);
    EXPECT_EQ(min_fee, 165413u);

    uint32_t total_input = send_amount() - 1000 + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), send_amount() - 1000);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount() - 1000);
    EXPECT_FALSE(tx->ChangeOutput());
  }

  // Sending one tenth of send_amount fails min value req.
  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 227u);
    EXPECT_EQ(min_fee, 165369u);

    uint32_t total_input = send_amount() / 10 + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input, 0));
    CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_FALSE(tx.has_value());

    EXPECT_EQ(tx.error(), WalletInsufficientBalanceErrorMessage());
  }
}

TEST_F(CardanoMaxSendSolverUnitTest, ManyInputs) {
  auto base_tx = MakeMockTransaction();

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 13884u);
  EXPECT_EQ(min_fee, 766277u);

  {
    uint32_t total_input = 100 * send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(send_amount() + min_fee, 0));
    for (int i = 1; i < 100; ++i) {
      inputs.push_back(MakeMockTxInput(send_amount(), i));
    }
    CardanoMaxSendSolver solver(base_tx, latest_epoch_parameters(), inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->inputs().size(), 100u);
    EXPECT_EQ(tx->TotalOutputsAmount(), 100 * send_amount());
    EXPECT_EQ(tx->TargetOutput()->amount, 100 * send_amount());
    EXPECT_FALSE(tx->ChangeOutput());
  }
}

}  // namespace brave_wallet
