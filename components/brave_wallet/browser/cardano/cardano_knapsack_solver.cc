/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <algorithm>
#include <utility>

#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr int kCardanoKnapsackSolverIterations = 1000;

uint64_t ApplyFeeRate(uint64_t min_fee_coefficient,
                      uint64_t min_fee_constant,
                      uint32_t tx_size) {
  return tx_size * min_fee_coefficient + min_fee_constant;
}

}  // namespace

CardanoKnapsackSolver::CardanoKnapsackSolver(
    CardanoTransaction base_transaction,
    uint64_t min_fee_coefficient,
    uint64_t min_fee_constant,
    std::vector<CardanoTransaction::TxInput> inputs)
    : base_transaction_(std::move(base_transaction)),
      min_fee_coefficient_(min_fee_coefficient),
      min_fee_constant_(min_fee_constant),
      inputs_(std::move(inputs)) {}
CardanoKnapsackSolver::~CardanoKnapsackSolver() = default;

// static
uint64_t CardanoKnapsackSolver::GetCostOfChangeOutput(
    const CardanoTransaction& tx,
    uint64_t min_fee_coefficient,
    uint64_t min_fee_constant) {
  CHECK(tx.ChangeOutput());

  auto fee_with_change =
      ApplyFeeRate(min_fee_coefficient, min_fee_constant,
                   CardanoSerializer::CalcTransactionSize(tx));

  auto tx_without_change = tx;
  tx_without_change.ClearChangeOutput();
  auto fee_without_change =
      ApplyFeeRate(min_fee_coefficient, min_fee_constant,
                   CardanoSerializer::CalcTransactionSize(tx_without_change));

  if (fee_with_change > fee_without_change) {
    return fee_with_change - fee_without_change;
  }
  return 0;
}

void CardanoKnapsackSolver::SolveForTransaction(
    const CardanoTransaction& transaction,
    std::optional<CardanoTransaction>& solution) {
  // Don't create transaction if output's amount appears to be less than this
  // threshold. Cost of spending such output should not be higher than its
  // value.
  uint64_t dust_output_threshold =
      transaction.ChangeOutput()
          ? GetCostOfChangeOutput(transaction, min_fee_coefficient_,
                                  min_fee_constant_)
          : 0;

  for (int i = 0; i < kCardanoKnapsackSolverIterations; ++i) {
    std::vector<bool> picked_groups(inputs_.size(), false);
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
            pass == 0 ? base::RandInt(0, 1) : !picked_groups[input_index];
        if (!pick_group) {
          continue;
        }

        CardanoTransaction next_transaction = cur_transaction;
        next_transaction.AddInput(inputs_[input_index]);

        // Minimum fee required for this transaction to be accepted.
        // Depends on transaction's size and current fee rate.
        uint64_t min_fee = ApplyFeeRate(
            min_fee_coefficient_, min_fee_constant_,
            CardanoSerializer::CalcTransactionSize(next_transaction));

        // Move everything except `min_fee` to change output(if any). Throw
        // away possible transaction if resulting change amount is less than
        // dust threshold.
        if (auto change_amount =
                next_transaction.MoveSurplusFeeToChangeOutput(min_fee)) {
          if (change_amount < dust_output_threshold) {
            continue;
          }
        }

        if (next_transaction.AmountsAreValid(min_fee)) {
          has_valid_transaction_for_iteration = true;
          if (!solution || solution->EffectiveFeeAmount() <
                               next_transaction.EffectiveFeeAmount()) {
            solution = next_transaction;
          }
        } else {
          picked_groups[input_index] = true;
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

  std::optional<CardanoTransaction> solution;

  // Try to find the best transaction with a change output which receives a
  // fee surplus.
  SolveForTransaction(base_transaction_, solution);

  // Drop the change output from the transaction and try to find the best
  // transaction again. Might find a transaction with a slightly higher fee
  // but still less than the cost of having a change output.
  auto no_change_transaction = base_transaction_;
  no_change_transaction.ClearChangeOutput();
  SolveForTransaction(no_change_transaction, solution);

  if (!solution) {
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE));
  }

  return base::ok(std::move(*solution));
}

}  // namespace brave_wallet
