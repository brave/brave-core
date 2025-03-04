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
// `SolveForTransaction` for details.
// TODO(apaymyshev): consider moving this calculation to separate thread.
class CardanoKnapsackSolver {
 public:
  CardanoKnapsackSolver(CardanoTransaction base_transaction,
                        uint64_t min_fee_coefficient,
                        uint64_t min_fee_constant,
                        std::vector<CardanoTransaction::TxInput> inputs);
  ~CardanoKnapsackSolver();

  static uint64_t GetCostOfChangeOutput(const CardanoTransaction& tx,
                                        uint64_t min_fee_coefficient,
                                        uint64_t min_fee_constant);

  base::expected<CardanoTransaction, std::string> Solve();

 private:
  CardanoTransaction base_transaction_;
  uint64_t min_fee_coefficient_ = 0;
  uint64_t min_fee_constant_ = 0;
  std::vector<CardanoTransaction::TxInput> inputs_;

  void SolveForTransaction(const CardanoTransaction& transaction,
                           std::optional<CardanoTransaction>& solution);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_KNAPSACK_SOLVER_H_
