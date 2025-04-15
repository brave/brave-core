/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <algorithm>
#include <utility>

#include "base/numerics/clamped_math.h"
#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
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

// static
uint64_t GetCostOfChangeOutput(const CardanoTransaction& tx,
                               uint64_t min_fee_coefficient,
                               uint64_t min_fee_constant) {
  CHECK(tx.ChangeOutput());

  auto tx_with_change = tx;
  CardanoTransaction::TxInput fake_input;
  fake_input.utxo_value = tx.amount();
  tx_with_change.AddInput(std::move(fake_input));

  auto fee_with_change =
      ApplyFeeRate(min_fee_coefficient, min_fee_constant,
                   CardanoSerializer::CalcTransactionSize(tx_with_change));

  auto tx_without_change = tx_with_change;
  tx_without_change.ClearChangeOutput();
  auto fee_without_change =
      ApplyFeeRate(min_fee_coefficient, min_fee_constant,
                   CardanoSerializer::CalcTransactionSize(tx_without_change));

  return base::ClampSub(fee_with_change, fee_without_change);
}

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

  // Don't create transaction if output's amount appears to be less than this
  // threshold. Cost of spending such output should not be higher than its
  // value.
  uint64_t dust_output_threshold =
      transaction.ChangeOutput()
          ? GetCostOfChangeOutput(transaction,
                                  latest_epoch_parameters_.min_fee_coefficient,
                                  latest_epoch_parameters_.min_fee_constant)
          : 0;

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
        next_transaction.AddWitness(
            CardanoTransaction::TxWitness::DummyTxWitness());

        // Minimum fee required for this transaction to be accepted.
        // Depends on transaction's size and current fee rate.
        uint64_t min_fee = ApplyFeeRate(
            latest_epoch_parameters_.min_fee_coefficient,
            latest_epoch_parameters_.min_fee_constant,
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
          if (!current_best_solution_ ||
              current_best_solution_->EffectiveFeeAmount() >
                  next_transaction.EffectiveFeeAmount()) {
            current_best_solution_ = next_transaction;
          }
        } else {
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
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE));
  }

  // Clear dummy witnesses used for tx size calculation.
  current_best_solution_->SetWitnesses({});

  return base::ok(std::move(*current_best_solution_));
}

}  // namespace brave_wallet
