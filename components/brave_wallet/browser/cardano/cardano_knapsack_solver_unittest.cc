/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <utility>

#include "base/containers/span.h"
#include "base/rand_util.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

uint64_t MinFeeForTxSize(uint64_t min_fee_coefficient,
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
  TxBuilderParms MakeTxBuilderParams(uint64_t amount) {
    TxBuilderParms builder_params;
    builder_params.amount = amount;
    builder_params.sending_max_amount = false;
    builder_params.send_to_address = *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                1, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, 123))
            ->address_string);
    builder_params.change_address = GetChangeAddress();
    builder_params.epoch_parameters = latest_epoch_parameters();
    builder_params.invalid_after = 12345;

    return builder_params;
  }

  TxBuilderParms MakeTxBuilderParams(uint64_t amount,
                                     cardano_rpc::TokenId token_id) {
    auto params = MakeTxBuilderParams(amount);
    params.token_to_send = token_id;
    return params;
  }

  CardanoTransaction::TxInput MakeMockTxInput(uint64_t amount) {
    uint32_t id = next_input_id_++;
    auto address = keyring_
                       .GetAddress(0, mojom::CardanoKeyId(
                                          mojom::CardanoKeyRole::kExternal, 0))
                       ->address_string;

    CardanoTransaction::TxInput tx_input;
    tx_input.utxo_address = *CardanoAddress::FromString(address);
    tx_input.utxo_outpoint.txid =
        crypto::hash::Sha256(base::byte_span_from_ref(id));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid[0];
    tx_input.utxo_value = amount;

    return tx_input;
  }

  uint64_t send_amount() const { return 1'000'001; }
  uint64_t min_fee_coefficient() const { return 44; }
  uint64_t min_fee_constant() const { return 155381; }
  uint64_t coins_per_utxo_size() const { return 4310; }
  cardano_rpc::EpochParameters latest_epoch_parameters() {
    return cardano_rpc::EpochParameters{
        .min_fee_coefficient = min_fee_coefficient(),
        .min_fee_constant = min_fee_constant(),
        .coins_per_utxo_size = coins_per_utxo_size()};
  }
  uint32_t dust_change_threshold() const { return 969750; }

  CardanoAddress GetChangeAddress() {
    return *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, 456))
            ->address_string);
  }

  CardanoHDKeyring keyring_{*bip39::MnemonicToSeed(kMnemonicAbandonAbandon),
                            mojom::KeyringId::kCardanoMainnet};

  uint32_t next_input_id_ = 30;
};

TEST_F(CardanoKnapsackSolverUnitTest, SetupOutput) {
  auto builder_params = MakeTxBuilderParams(500'000);

  CardanoTransaction tx;
  tx.set_invalid_after(builder_params.invalid_after);
  // Send amount is less than min ada required for output
  EXPECT_EQ(CardanoKnapsackSolver::SetupOutput(tx, builder_params).error(),
            WalletAmountTooSmallErrorMessage());

  builder_params = MakeTxBuilderParams(send_amount());
  tx = {};
  EXPECT_TRUE(
      CardanoKnapsackSolver::SetupOutput(tx, builder_params).has_value());
  EXPECT_EQ(tx.TargetOutput()->amount, send_amount());
  EXPECT_EQ(tx.TargetOutput()->tokens, cardano_rpc::Tokens());
  EXPECT_FALSE(tx.ChangeOutput());

  builder_params = MakeTxBuilderParams(10, GetMockTokenId("foo"));
  tx = {};
  tx.set_invalid_after(builder_params.invalid_after);

  EXPECT_TRUE(
      CardanoKnapsackSolver::SetupOutput(tx, builder_params).has_value());
  EXPECT_EQ(tx.TargetOutput()->amount,
            1133530u);  // Min ada required for an output with a token.
  EXPECT_EQ(tx.TargetOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("foo"), 10}}));
  EXPECT_FALSE(tx.ChangeOutput());
}

TEST_F(CardanoKnapsackSolverUnitTest, SortInputs) {
  auto builder_params = MakeTxBuilderParams(send_amount());

  std::vector<CardanoTransaction::TxInput> inputs;
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 1'000'000'000;
  inputs.back().utxo_tokens[bar_token] = 1'000'000'000;
  inputs.back().utxo_tokens[baz_token] = 1'000'000'000;

  inputs.push_back(MakeMockTxInput(2'000'000));
  inputs.back().utxo_tokens[foo_token] = 10;
  inputs.back().utxo_tokens[bar_token] = 100'000'000;

  inputs.push_back(MakeMockTxInput(3'000'000));
  inputs.back().utxo_tokens[foo_token] = 1;

  inputs.push_back(MakeMockTxInput(4'000'000));

  auto original = inputs;
  CardanoKnapsackSolver::SortInputs(inputs, builder_params);
  EXPECT_THAT(inputs, testing::ElementsAre(original[3], original[2],
                                           original[1], original[0]));

  builder_params.token_to_send = bar_token;

  CardanoKnapsackSolver::SortInputs(inputs, builder_params);
  EXPECT_THAT(inputs, testing::ElementsAre(original[0], original[1],
                                           original[3], original[2]));
}

TEST_F(CardanoKnapsackSolverUnitTest, NoInputs) {
  auto builder_params = MakeTxBuilderParams(send_amount());

  CardanoKnapsackSolver solver(builder_params, {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NoInputs_TokenSend) {
  auto builder_params =
      MakeTxBuilderParams(send_amount(), GetMockTokenId("brave"));

  CardanoKnapsackSolver solver(builder_params, {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NotEnoughInputsForFee) {
  auto builder_params = MakeTxBuilderParams(send_amount());

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(send_amount()));
  CardanoKnapsackSolver solver(builder_params, inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NotEnoughTokensToSend) {
  auto builder_params = MakeTxBuilderParams(10, GetMockTokenId("brave"));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(10));
  inputs.back().utxo_tokens[GetMockTokenId("brave")] = 5;
  CardanoKnapsackSolver solver(builder_params, inputs);

  // Can't send exact amount of coin we have as we need to add some fee.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoKnapsackSolverUnitTest, NoChangeGenerated) {
  auto builder_params = MakeTxBuilderParams(send_amount());

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 230u);
  EXPECT_EQ(min_fee, 165501u);

  {
    uint32_t total_input = send_amount() + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
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

  {
    uint32_t total_input = send_amount() + min_fee - 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    // We have a bit less than send amount + fee. Can't create transaction.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }

  {
    uint32_t total_input = send_amount() + min_fee + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();

    // We have a bit more than send amount + min_fee. Can't create transaction
    // as it would require us raising fee, or creating change output which would
    // have less than min utxo limit.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, NoChangeGenerated_TokenSend) {
  auto builder_params = MakeTxBuilderParams(10, GetMockTokenId("brave"));

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 270u);
  EXPECT_EQ(min_fee, 167261u);
  uint32_t min_ada = 1142150u;  // Min ada for output having specified token.

  {
    uint32_t total_input = min_ada + min_fee;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(min_ada + min_fee));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have exactly send amount + fee.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), min_ada);
    EXPECT_EQ(tx->TargetOutput()->amount, min_ada);
    EXPECT_FALSE(tx->ChangeOutput());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  {
    uint32_t total_input = min_ada + min_fee - 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    // We have a bit less than min_ada + fee. Can't create transaction.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }

  {
    uint32_t total_input = min_ada + min_fee + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();

    // We have a bit more than min_ada + min_fee. Can't create transaction
    // as it would require us raising fee, or creating change output which would
    // have less than min utxo limit.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, NoDustChangeGenerated) {
  auto builder_params = MakeTxBuilderParams(send_amount());

  // Fee for typical 1 input -> 2 outputs transaction.
  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 295u);
  EXPECT_EQ(min_fee, 168361u);

  {
    uint32_t total_input = send_amount() + min_fee + dust_change_threshold();
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // Change output is created and has exactly `dust_change_threshold` amount.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(),
              send_amount() + dust_change_threshold());
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  {
    uint32_t total_input =
        send_amount() + min_fee + dust_change_threshold() - 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();

    // We have slightly less than needed for change output, so it is not created
    // and we don't allow surplus going to fee.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }

  {
    uint32_t total_input =
        send_amount() + min_fee + dust_change_threshold() + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly more than needed for change output which all goes to
    // change.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(),
              send_amount() + dust_change_threshold() + 1);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold() + 1);
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, NoDustChangeGenerated_TokenSend) {
  auto builder_params = MakeTxBuilderParams(10, GetMockTokenId("brave"));

  // Fee for typical 1 input -> 2 outputs transaction.
  uint32_t min_fee =
      MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 335u);
  EXPECT_EQ(min_fee, 170121u);
  uint32_t min_ada = 1142150;  // Min ada for output having specified token.

  {
    uint32_t total_input = min_ada + min_fee + dust_change_threshold();
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // Change output is created and has exactly `dust_change_threshold` amount.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(),
              min_ada + dust_change_threshold());
    EXPECT_EQ(tx->TargetOutput()->amount, min_ada);
    EXPECT_EQ(tx->TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 10}}));
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold());
    EXPECT_EQ(tx->ChangeOutput()->tokens, cardano_rpc::Tokens());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  {
    uint32_t total_input = min_ada + min_fee + dust_change_threshold() - 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();

    // We have slightly less than needed for change output, so it is not created
    // and we don't allow surplus going to fee.
    ASSERT_FALSE(tx.has_value());
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), tx.error());
  }

  {
    uint32_t total_input = min_ada + min_fee + dust_change_threshold() + 1;
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(total_input));
    inputs.back().utxo_tokens[GetMockTokenId("brave")] = 10;
    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    // We have slightly more than needed for change output which all goes to
    // change.
    EXPECT_EQ(tx->fee(), min_fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - min_fee);
    EXPECT_EQ(tx->TargetOutput()->amount, min_ada);
    EXPECT_EQ(tx->TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 10}}));
    EXPECT_EQ(tx->ChangeOutput()->amount, dust_change_threshold() + 1);
    EXPECT_EQ(tx->ChangeOutput()->tokens, cardano_rpc::Tokens());
    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, RandomTest) {
  std::vector<CardanoTransaction::TxInput> inputs;

  uint64_t total_inputs = 0;

  for (int i = 0; i < 50; ++i) {
    auto input = MakeMockTxInput(base::RandInt(1000000, 100000000));
    total_inputs += input.utxo_value;
    inputs.push_back(std::move(input));
  }

  auto builder_params = MakeTxBuilderParams(total_inputs / 2);

  CardanoKnapsackSolver solver(builder_params, inputs);
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
  EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
      *tx, latest_epoch_parameters()));
}

TEST_F(CardanoKnapsackSolverUnitTest, TokensGoToChange) {
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
    uint32_t total_input = 1'000'000 * 3;

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 447u);
    EXPECT_EQ(fee, 175049u);

    auto builder_params = MakeTxBuilderParams(send_amount());

    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_TRUE(tx->TargetOutput()->tokens.empty());
    EXPECT_EQ(tx->ChangeOutput()->amount, total_input - send_amount() - fee);

    // First input is not picked as it would produce larger tx and require more
    // fee.
    EXPECT_FALSE(tx->ChangeOutput()->tokens.empty());
    EXPECT_EQ(tx->ChangeOutput()->tokens[foo_token], 11u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[bar_token], 100'000'000u);

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }

  // Remove input with no tokens.
  inputs.pop_back();

  {
    uint32_t total_input = 1'000'000 * 3;

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 460u);
    EXPECT_EQ(fee, 175621u);

    auto builder_params = MakeTxBuilderParams(send_amount());

    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount, send_amount());
    EXPECT_TRUE(tx->TargetOutput()->tokens.empty());
    EXPECT_EQ(tx->ChangeOutput()->amount, total_input - send_amount() - fee);

    // First input is now picked.
    EXPECT_FALSE(tx->ChangeOutput()->tokens.empty());
    EXPECT_EQ(tx->ChangeOutput()->tokens[foo_token], 1'000'000'011u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[bar_token], 1'100'000'000u);
    EXPECT_EQ(tx->ChangeOutput()->tokens[baz_token], 1'000'000'000u);

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

TEST_F(CardanoKnapsackSolverUnitTest, TokensGoToChange_TokenSend) {
  auto foo_token = GetMockTokenId("foo");
  auto bar_token = GetMockTokenId("bar");
  auto baz_token = GetMockTokenId("baz");

  std::vector<CardanoTransaction::TxInput> inputs;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 1'000;
  inputs.back().utxo_tokens[bar_token] = 1'000;
  inputs.back().utxo_tokens[baz_token] = 1'000;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 10;
  inputs.back().utxo_tokens[bar_token] = 1'000;

  inputs.push_back(MakeMockTxInput(1'000'000));
  inputs.back().utxo_tokens[foo_token] = 5;

  {
    uint32_t total_input = 1'000'000 * 3;

    uint32_t fee =
        MinFeeForTxSize(min_fee_coefficient(), min_fee_constant(), 492u);
    EXPECT_EQ(fee, 177029u);
    uint32_t min_ada = 1142150u;  // Min ada for output having specified token.

    auto builder_params = MakeTxBuilderParams(1'011, foo_token);

    CardanoKnapsackSolver solver(builder_params, inputs);
    auto tx = solver.Solve();
    ASSERT_TRUE(tx.has_value());

    EXPECT_EQ(tx->fee(), fee);
    EXPECT_EQ(tx->GetTotalInputsAmount().ValueOrDie(), total_input);
    EXPECT_EQ(tx->GetTotalOutputsAmount().ValueOrDie(), total_input - fee);
    EXPECT_EQ(tx->TargetOutput()->amount, min_ada);
    EXPECT_EQ(tx->TargetOutput()->tokens,
              cardano_rpc::Tokens({{foo_token, 1'011u}}));
    EXPECT_EQ(tx->ChangeOutput()->amount, total_input - min_ada - fee);
    // Change output gets change for foo_token and all other tokens.
    EXPECT_EQ(tx->ChangeOutput()->tokens,
              cardano_rpc::Tokens(
                  {{foo_token, 4u}, {bar_token, 2'000u}, {baz_token, 1'000u}}));

    EXPECT_TRUE(CardanoTransactionSerializer::ValidateAmounts(
        *tx, latest_epoch_parameters()));
  }
}

}  // namespace brave_wallet
