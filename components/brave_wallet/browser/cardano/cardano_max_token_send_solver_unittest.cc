/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_token_send_solver.h"

#include <algorithm>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/insecure_random_generator.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace brave_wallet {

namespace {

class SeededBitGenerator {
 public:
  using result_type = uint64_t;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return UINT64_MAX; }
  result_type operator()() const { return rnd_.RandUint64(); }

  explicit SeededBitGenerator(uint64_t seed) { rnd_.ReseedForTesting(seed); }
  ~SeededBitGenerator() = default;

 private:
  base::test::InsecureRandomGenerator rnd_;
};

}  // namespace

class CardanoMaxTokenSendSolverUnitTest : public testing::Test {
 public:
  CardanoMaxTokenSendSolverUnitTest() = default;
  ~CardanoMaxTokenSendSolverUnitTest() override = default;

 protected:
  TxBuilderParms MakeTxBuilderParams(const cardano_rpc::TokenId& token_id) {
    TxBuilderParms builder_params(GetSendToAddress(), GetChangeAddress());
    builder_params.invalid_after = 12345;
    builder_params.epoch_parameters = latest_epoch_parameters();
    builder_params.amount = 0;
    builder_params.sending_max_amount = true;
    builder_params.token_to_send = token_id;

    return builder_params;
  }

  CardanoTransaction::TxInput MakeMockTxInput(uint64_t amount) {
    auto address = keyring_
                       .GetAddress(0, mojom::CardanoKeyId(
                                          mojom::CardanoKeyRole::kExternal, 0))
                       ->address_string;

    uint32_t id = next_input_id_++;
    CardanoTransaction::TxInput tx_input(*CardanoAddress::FromString(address));
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
    CardanoTransaction::TxInput tx_input(*CardanoAddress::FromString(address));
    tx_input.utxo_outpoint.txid =
        crypto::hash::Sha256(base::byte_span_from_ref(id));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid[0];
    tx_input.utxo_value = amount;
    tx_input.utxo_tokens = tokens;

    return tx_input;
  }

  CardanoAddress GetSendToAddress() {
    return *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                1, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, 123))
            ->address_string);
  }

  CardanoAddress GetChangeAddress() {
    return *CardanoAddress::FromString(
        keyring_
            .GetAddress(
                0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, 456))
            ->address_string);
  }

  uint64_t min_fee_coefficient() const { return 44; }
  uint64_t min_fee_constant() const { return 155'381; }
  uint64_t coins_per_utxo_size() const { return 4'310; }

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

TEST_F(CardanoMaxTokenSendSolverUnitTest, SetupOutputs) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));
  CardanoTransaction tx;
  tx.set_invalid_after(builder_params.invalid_after);

  // No brave token.
  EXPECT_FALSE(CardanoMaxTokenSendSolver::SetupOutputs(tx, builder_params));
  tx.AddInput(MakeMockTxInput(3'000'000u, {{GetMockTokenId("baz"), 123}}));
  EXPECT_FALSE(CardanoMaxTokenSendSolver::SetupOutputs(tx, builder_params));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 3},
                                                {GetMockTokenId("foo"), 300}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 4},
                                                {GetMockTokenId("bar"), 777}}));
  tx.AddInputs(inputs);

  EXPECT_FALSE(tx.TargetOutput());
  EXPECT_FALSE(tx.ChangeOutput());
  EXPECT_TRUE(CardanoMaxTokenSendSolver::SetupOutputs(tx, builder_params));
  EXPECT_EQ(tx.TargetOutput()->type, CardanoTransaction::TxOutputType::kTarget);
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);  // min ADA required.
  EXPECT_EQ(tx.TargetOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("brave"), 9}}));
  EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
  EXPECT_EQ(tx.ChangeOutput()->amount, 0u);
  EXPECT_EQ(tx.ChangeOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("foo"), 300},
                                 {GetMockTokenId("bar"), 777},
                                 {GetMockTokenId("baz"), 123}}));
  EXPECT_EQ(tx.fee(), 0u);
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, ExtractTokenInputs) {
  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 3},
                                                {GetMockTokenId("foo"), 300}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("baz"), 123}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 4},
                                                {GetMockTokenId("bar"), 777}}));

  auto [token_inputs, other_inputs] =
      CardanoMaxTokenSendSolver::SplitInputsByToken(GetMockTokenId("brave"),
                                                    inputs);

  EXPECT_THAT(token_inputs, ElementsAre(inputs[0], inputs[1], inputs[3]));
  EXPECT_THAT(other_inputs, ElementsAre(inputs[2]));
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, SortInputs) {
  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(4'000'000u, {{GetMockTokenId("brave"), 3},
                                                {GetMockTokenId("foo"), 300}}));
  inputs.push_back(MakeMockTxInput(5'000'000u, {{GetMockTokenId("baz"), 123}}));
  inputs.push_back(MakeMockTxInput(6'000'000u, {{GetMockTokenId("brave"), 4},
                                                {GetMockTokenId("bar"), 777}}));
  inputs.push_back(MakeMockTxInput(7'000'000u, {}));
  inputs.push_back(MakeMockTxInput(10'000'000u, {}));

  auto original = inputs;
  CardanoMaxTokenSendSolver::SortInputsBySelectionPriority(inputs);

  EXPECT_THAT(inputs, ElementsAre(original[5], original[4], original[2],
                                  original[0], original[3], original[1]));
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, NoInputs) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));

  CardanoMaxTokenSendSolver solver(builder_params, {});

  // Can't send when there is no inputs.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, NoTokenToSend) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(2'000'000u));
  CardanoMaxTokenSendSolver solver(builder_params, inputs);

  // Can't send brave token as there is no containing inputs.
  EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, NotEnoughLovelaceForTwoOutputs) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));
  uint64_t lovelace_amount = 2'282'021;

  // Can't spend this input as it is not enough for two outputs.
  {
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(
        MakeMockTxInput(lovelace_amount - 1, {{GetMockTokenId("brave"), 1}}));
    CardanoMaxTokenSendSolver solver(builder_params, inputs);

    // Can't spend this input as it is not enough for two outputs.
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
  }

  // Amount of lovelaces is now enough to create transaction.
  {
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(
        MakeMockTxInput(lovelace_amount, {{GetMockTokenId("brave"), 1}}));
    CardanoMaxTokenSendSolver solver(builder_params, inputs);

    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens, inputs[0].utxo_tokens);
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 969'750u);
    EXPECT_TRUE(tx.ChangeOutput()->tokens.empty());
    EXPECT_EQ(tx.fee(), 170'121u);
  }

  // Exceeding amount of lovelaces goes to change.
  {
    std::vector<CardanoTransaction::TxInput> inputs;
    inputs.push_back(MakeMockTxInput(lovelace_amount + 1000,
                                     {{GetMockTokenId("brave"), 1}}));
    CardanoMaxTokenSendSolver solver(builder_params, inputs);

    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens, inputs[0].utxo_tokens);
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 969'750u + 1'000u);
    EXPECT_TRUE(tx.ChangeOutput()->tokens.empty());
    EXPECT_EQ(tx.fee(), 170'121u);
  }
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, TwoInputsWithToken) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 3}}));
  CardanoMaxTokenSendSolver solver(builder_params, inputs);

  auto tx = solver.Solve().value();
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
  EXPECT_EQ(tx.TargetOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("brave"), 5}}));
  EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
  EXPECT_EQ(tx.ChangeOutput()->amount, 4'686'101u);
  EXPECT_TRUE(tx.ChangeOutput()->tokens.empty());
  EXPECT_EQ(tx.fee(), 171'749u);
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, TokensGoToChange) {
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 2}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 3},
                                                {GetMockTokenId("foo"), 300}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("brave"), 4},
                                                {GetMockTokenId("bar"), 777}}));
  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("baz"), 123}}));
  CardanoMaxTokenSendSolver solver(builder_params, inputs);

  // Non-brave tokens go to change. Input with baz token is not picked as
  // sum lovelace amount from the first three inputs is enough to cover fee.
  auto tx = solver.Solve().value();
  EXPECT_EQ(tx.inputs().size(), 3u);
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
  EXPECT_EQ(tx.TargetOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("brave"), 9}}));
  EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
  EXPECT_EQ(tx.ChangeOutput()->amount, 7'681'041u);
  EXPECT_EQ(tx.ChangeOutput()->tokens,
            cardano_rpc::Tokens(
                {{GetMockTokenId("foo"), 300}, {GetMockTokenId("bar"), 777}}));
  EXPECT_EQ(tx.fee(), 176'809u);
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, ExtraInputsPickOrder) {
  SeededBitGenerator rnd(1);
  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave"));

  std::vector<CardanoTransaction::TxInput> inputs;
  inputs.push_back(MakeMockTxInput(1'142'150u, {{GetMockTokenId("brave"), 5}}));
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  // Not enough lovelace to create transaction.
  {
    CardanoMaxTokenSendSolver solver(builder_params, inputs);
    EXPECT_EQ(WalletInsufficientBalanceErrorMessage(), solver.Solve().error());
  }

  inputs.push_back(MakeMockTxInput(
      3'000'000u, {{GetMockTokenId("foo"), 5}, {GetMockTokenId("bar"), 7}}));
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  // Pick both inputs. Foo and bar tokens go to change.
  {
    CardanoMaxTokenSendSolver solver(builder_params, inputs);
    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.inputs().size(), 2u);
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 5}}));
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 2'824'995u);
    EXPECT_EQ(tx.ChangeOutput()->tokens,
              cardano_rpc::Tokens(
                  {{GetMockTokenId("foo"), 5}, {GetMockTokenId("bar"), 7}}));
    EXPECT_EQ(tx.fee(), 175'005u);
  }

  inputs.push_back(MakeMockTxInput(3'000'000u, {{GetMockTokenId("foo"), 5}}));
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  // Prefer new input with less tokens.
  {
    CardanoMaxTokenSendSolver solver(builder_params, inputs);
    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.inputs().size(), 2u);
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 5}}));
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 2'826'579u);
    EXPECT_EQ(tx.ChangeOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("foo"), 5}}));
    EXPECT_EQ(tx.fee(), 173'421u);
  }

  inputs.push_back(MakeMockTxInput(3'000'000u));
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  // Prefer new input with no tokens.
  {
    CardanoMaxTokenSendSolver solver(builder_params, inputs);
    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.inputs().size(), 2u);
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 5}}));
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 2'828'251u);
    EXPECT_EQ(tx.ChangeOutput()->tokens, cardano_rpc::Tokens({}));
    EXPECT_EQ(tx.fee(), 171'749u);
  }

  inputs.push_back(MakeMockTxInput(5'000'000u));
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  // Prefer new input with larger lovelace.
  {
    CardanoMaxTokenSendSolver solver(builder_params, inputs);
    auto tx = solver.Solve().value();
    EXPECT_EQ(tx.inputs().size(), 2u);
    EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
    EXPECT_EQ(tx.TargetOutput()->amount, 1'142'150u);
    EXPECT_EQ(tx.TargetOutput()->tokens,
              cardano_rpc::Tokens({{GetMockTokenId("brave"), 5}}));
    EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
    EXPECT_EQ(tx.ChangeOutput()->amount, 4'828'251u);
    EXPECT_EQ(tx.ChangeOutput()->tokens, cardano_rpc::Tokens({}));
    EXPECT_EQ(tx.fee(), 171'749u);
  }
}

TEST_F(CardanoMaxTokenSendSolverUnitTest, ManyInputsManyTokens) {
  SeededBitGenerator rnd(1);

  // Randomly generate set of inputs each having set of tokens.
  std::vector<CardanoTransaction::TxInput> inputs;
  for (auto i = 0; i < 100; ++i) {
    inputs.push_back(MakeMockTxInput(10'000'000u));

    int n_tokens = rnd() % 10;
    for (auto t = 0; t < n_tokens; ++t) {
      inputs.back().utxo_tokens[GetMockTokenId(
          "brave_" + base::NumberToString(rnd() % 20))] = 1 + rnd() % 1000;
    }
  }
  std::shuffle(inputs.begin(), inputs.end(), rnd);

  auto builder_params = MakeTxBuilderParams(GetMockTokenId("brave_0"));
  CardanoMaxTokenSendSolver solver(builder_params, inputs);
  auto tx = solver.Solve().value();
  EXPECT_EQ(tx.inputs().size(), 24u);
  EXPECT_EQ(tx.TargetOutput()->address, builder_params.send_to_address);
  EXPECT_EQ(tx.TargetOutput()->amount, 1'159'390u);
  EXPECT_EQ(tx.TargetOutput()->tokens,
            cardano_rpc::Tokens({{GetMockTokenId("brave_0"), 11898}}));
  EXPECT_EQ(tx.ChangeOutput()->address, builder_params.change_address);
  EXPECT_EQ(tx.ChangeOutput()->amount, 238'621'737u);
  EXPECT_EQ(tx.ChangeOutput()->tokens, cardano_rpc::Tokens({
                                           {GetMockTokenId("brave_1"), 2276},
                                           {GetMockTokenId("brave_10"), 3669},
                                           {GetMockTokenId("brave_11"), 6441},
                                           {GetMockTokenId("brave_12"), 5058},
                                           {GetMockTokenId("brave_13"), 5044},
                                           {GetMockTokenId("brave_14"), 2737},
                                           {GetMockTokenId("brave_15"), 1941},
                                           {GetMockTokenId("brave_16"), 1810},
                                           {GetMockTokenId("brave_17"), 2395},
                                           {GetMockTokenId("brave_18"), 2009},
                                           {GetMockTokenId("brave_19"), 3676},
                                           {GetMockTokenId("brave_2"), 4623},
                                           {GetMockTokenId("brave_3"), 1520},
                                           {GetMockTokenId("brave_4"), 868},
                                           {GetMockTokenId("brave_5"), 2402},
                                           {GetMockTokenId("brave_6"), 1383},
                                           {GetMockTokenId("brave_7"), 2369},
                                           {GetMockTokenId("brave_8"), 3076},
                                           {GetMockTokenId("brave_9"), 6028},
                                       }));
  EXPECT_EQ(tx.fee(), 218'873u);
}

}  // namespace brave_wallet
