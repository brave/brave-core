/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_KNAPSACK_SOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_KNAPSACK_SOLVER_H_

#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

namespace brave_wallet {

// Knapsack coin selection algorithm. Inspired by
// https://github.com/sparrowwallet/drongo/blob/master/src/main/java/com/sparrowwallet/drongo/wallet/KnapsackUtxoSelector.java#L8
// and
// https://github.com/bitcoin/bitcoin/blob/v25.1/src/wallet/coinselection.cpp#L255
// Tries to find the best set of inputs(minimal fee) for a transaction.
// Does two runs of search: with and without change output. See
// `RunSolverForTransaction` for details.
// TODO(https://github.com/brave/brave-browser/issues/45278): consider moving
// this calculation to separate thread.
class CardanoKnapsackSolver {
 public:
  CardanoKnapsackSolver(TxBuilderParms builder_params,
                        std::vector<CardanoTransaction::TxInput> inputs);
  ~CardanoKnapsackSolver();

  // Searches for best transaction(lesser fee).
  base::expected<CardanoTransaction, std::string> Solve();

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoKnapsackSolverUnitTest, SetupOutput);
  FRIEND_TEST_ALL_PREFIXES(CardanoKnapsackSolverUnitTest, SortInputs);
  // Setup target output. In case of lovelace transfer validate if amount fits
  // min output limit. In case of token send also set min required lovelace
  // amount.
  static base::expected<void, std::string> SetupOutput(
      CardanoTransaction& tx,
      const TxBuilderParms& builder_params);

  // Sort inputs by the token amount in descending order. If the token amount
  // is the same, sort by the utxo value in descending order.
  static void SortInputs(std::vector<CardanoTransaction::TxInput>& inputs,
                         const TxBuilderParms& builder_params);

  // Various params required to create transaction.
  TxBuilderParms builder_params_;

  // Set of possible inputs to pick for transaction.
  std::vector<CardanoTransaction::TxInput> inputs_;

  // Runs solver algorithm for the `transaction`.
  // Updates `current_best_solution` when transaction with lesser fee is found.
  void RunSolverForTransaction(
      const CardanoTransaction& transaction,
      std::optional<CardanoTransaction>& current_best_solution);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_KNAPSACK_SOLVER_H_
