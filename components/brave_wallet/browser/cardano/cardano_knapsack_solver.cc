/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

namespace brave_wallet {

namespace {

constexpr int kCardanoKnapsackSolverIterations = 1000;

}  // namespace

CardanoKnapsackSolver::CardanoKnapsackSolver(
    CardanoTransaction base_transaction,
    cardano_rpc::EpochParameters latest_epoch_parameters,
    std::vector<CardanoTransaction::TxInput> inputs)
    : base_transaction_(std::move(base_transaction)),
      latest_epoch_parameters_(std::move(latest_epoch_parameters)),
      inputs_(std::move(inputs)) {}
CardanoKnapsackSolver::~CardanoKnapsackSolver() = default;

void CardanoKnapsackSolver::RunSolverForTransaction(
    const CardanoTransaction& transaction) {
  if (inputs_.empty()) {
    return;
  }

  std::vector<uint8_t> picked_inputs(inputs_.size(), false);
  for (int i = 0; i < kCardanoKnapsackSolverIterations; ++i) {
    std::ranges::fill(picked_inputs, false);

    CardanoTransaction cur_transaction = transaction;

    bool has_valid_transaction_for_iteration = false;

    // First pass: Go through input groups(sorted desc) and randomly pick
    // them. If we have a valid transaction discard last group and continue
    // trying to make valid transactions with smaller groups.
    // Second pass(if no valid transactions from 1st pass): Forcedly pick
    // yet not picked groups(starting from largest).
    for (int pass = 0; pass < 2; ++pass) {
      if (has_valid_transaction_for_iteration) {
        DCHECK_EQ(pass, 1);
        break;
      }

      for (uint32_t input_index = 0; input_index < inputs_.size();
           ++input_index) {
        bool pick_group =
            pass == 0 ? base::RandInt(0, 1) : !picked_inputs[input_index];
        if (!pick_group) {
          continue;
        }

        CardanoTransaction next_transaction = cur_transaction;
        next_transaction.AddInput(inputs_[input_index]);

        // Given set of inputs and outputs try to find valid outputs' amounts
        // and tx fee.
        if (auto found_valid_tx =
                CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
                    next_transaction, latest_epoch_parameters_)) {
          // Found a valid tx with given inputs. Check if it is the best
          // tx so far and don't update `cur_transaction` as it doesn't make
          // sense to add new inputs into it.
          has_valid_transaction_for_iteration = true;
          if (!current_best_solution_ ||
              current_best_solution_->fee() > found_valid_tx->fee()) {
            current_best_solution_ = std::move(*found_valid_tx);
          }
        } else {
          // Could not find a valid tx with given inputs. proceed to the next
          // iteration with current input added to `cur_transaction`.
          picked_inputs[input_index] = true;
          cur_transaction = std::move(next_transaction);
        }
      }
    }
  }
}

base::expected<CardanoTransaction, std::string> CardanoKnapsackSolver::Solve() {
  DCHECK_EQ(base_transaction_.inputs().size(), 0u);
  DCHECK(base_transaction_.TargetOutput());
  DCHECK(base_transaction_.ChangeOutput());
  DCHECK(!base_transaction_.sending_max_amount());

  if (!CardanoTransactionSerializer().ValidateMinValue(
          *base_transaction_.TargetOutput(), latest_epoch_parameters_)) {
    return base::unexpected(WalletAmountTooSmallErrorMessage());
  }

  // Try to find the best transaction with a change output which receives a
  // fee surplus.
  RunSolverForTransaction(base_transaction_);

  // Drop the change output from the transaction and try to find the best
  // transaction again. Might find a transaction with a slightly higher fee
  // but still less than the cost of having a change output.
  auto no_change_transaction = base_transaction_;
  no_change_transaction.ClearChangeOutput();
  RunSolverForTransaction(no_change_transaction);

  if (!current_best_solution_) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  DCHECK(current_best_solution_->witnesses().empty());
  return base::ok(std::move(*current_best_solution_));
}

}  // namespace brave_wallet
