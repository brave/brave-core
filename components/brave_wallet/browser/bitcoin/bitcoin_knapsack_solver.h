/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_KNAPSACK_SOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_KNAPSACK_SOLVER_H_

#include <map>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"

namespace brave_wallet {

// Knapsack coin selection algorithm. Inspired by
// https://github.com/sparrowwallet/drongo/blob/master/src/main/java/com/sparrowwallet/drongo/wallet/KnapsackUtxoSelector.java#L8
// and
// https://github.com/bitcoin/bitcoin/blob/v25.1/src/wallet/coinselection.cpp#L255
// Tries to find the best set of inputs(minimal fee) for a transaction.
// Does two runs of search: with and without change output. See
// `SolveForTransaction` for details.
class KnapsackSolver {
 public:
  KnapsackSolver(
      BitcoinTransaction base_transaction,
      double fee_rate,
      double longterm_fee_rate,
      const std::vector<BitcoinTransaction::TxInputGroup>& input_groups);
  ~KnapsackSolver();

  static uint64_t GetCostOfChangeOutput(
      const BitcoinTransaction::TxOutput& output,
      double fee_rate,
      double longterm_fee_rate);

  base::expected<BitcoinTransaction, std::string> Solve();

 private:
  BitcoinTransaction base_transaction_;
  double fee_rate_ = 0;
  double longterm_fee_rate_ = 0;
  std::vector<BitcoinTransaction::TxInputGroup> input_groups_;

  void SolveForTransaction(
      const BitcoinTransaction& transaction,
      std::multimap<uint64_t, BitcoinTransaction>& solutions);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_KNAPSACK_SOLVER_H_
