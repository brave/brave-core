/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_lovelace_send_solver.h"

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

class CardanoMaxLovelaceSendSolverUnitTest : public testing::Test {
 public:
  CardanoMaxLovelaceSendSolverUnitTest() = default;
  ~CardanoMaxLovelaceSendSolverUnitTest() override = default;

 protected:
  TxBuilderParms MakeTxBuilderParams() {
    TxBuilderParms builder_params;
    builder_params.send_to_address = *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                1, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, 123))
            ->address_string);
    builder_params.invalid_after = 12345;
    builder_params.epoch_parameters = latest_epoch_parameters();
    builder_params.amount = 0;
    builder_params.sending_max_amount = true;
    builder_params.change_address = GetChangeAddress();

    return builder_params;
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

  CardanoTransaction::TxInput MakeMockTxInput(
      uint64_t amount,
      const cardano_rpc::Tokens& tokens) {
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
    tx_input.utxo_tokens = tokens;

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

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, SetupOutputs) {
  auto builder_params = MakeTxBuilderParams();
  CardanoTransaction tx;
  tx.set_invalid_after(builder_params.invalid_after);

  // No inputs.
  EXPECT_TRUE(CardanoMaxLovelaceSendSolver::SetupOutputs(tx, builder_params));
  EXPECT_EQ(tx.TargetOutput()->type, CardanoTransaction::TxOutputType::kTarget);
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 0u);
  EXPECT_FALSE(tx.ChangeOutput());

  tx = {};
  tx.set_invalid_after(builder_params.invalid_after);

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("baz"), 123}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 3},
                                                {GetMockTokenId("foo"), 300}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 4},
                                                {GetMockTokenId("bar"), 777}}));
  tx.AddInputs(inputs);

  EXPECT_FALSE(tx.TargetOutput());
  EXPECT_FALSE(tx.ChangeOutput());
  EXPECT_TRUE(CardanoMaxLovelaceSendSolver::SetupOutputs(tx, builder_params));
  EXPECT_EQ(tx.TargetOutput()->type, CardanoTransaction::TxOutputType::kTarget);
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 0u);
  EXPECT_EQ(tx.TargetOutput()->tokens, cardano_rpc::Tokens());
  EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
  EXPECT_EQ(tx.ChangeOutput()->amount, 1'361'960u);  // min ADA required.
  EXPECT_EQ(tx.ChangeOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("brave"), 9},
                                 {GetMockTokenId("foo"), 300},
                                 {GetMockTokenId("bar"), 777},
                                 {GetMockTokenId("baz"), 123}}));
  EXPECT_EQ(tx.fee(), 0u);
}

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, NoInputs) {
  auto builder_params = MakeTxBuilderParams();

  CardanoMaxLovelaceSendSolver solver(builder_params, {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, NotEnoughInputsForFee) {
  auto builder_params = MakeTxBuilderParams();

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(send_amount()));
  CardanoMaxLovelaceSendSolver solver(builder_params, inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, NoChangeNeeded) {
  auto builder_params = MakeTxBuilderParams();

  {
    uint32_t min_fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
    EXPECT_EQ(min_fee, 165501u);

    uint32_t total_input = send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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
    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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
    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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
    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_FALSE(tx.has_value());

    EXPECT_EQ(tx.error(), WalletInsufficientBalanceErrorMessage());
  }
}

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, ManyInputs) {
  auto builder_params = MakeTxBuilderParams();

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
    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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

TEST_F(CardanoMaxLovelaceSendSolverUnitTest, TokensGoToChange) {
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

    auto builder_params = MakeTxBuilderParams();

    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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

    auto builder_params = MakeTxBuilderParams();

    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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

    auto builder_params = MakeTxBuilderParams();

    CardanoMaxLovelaceSendSolver solver(builder_params, inputs);
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
