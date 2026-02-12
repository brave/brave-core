/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/rand_util.h"
#include "base/types/expected.h"
#include "base/types/optional_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

namespace brave_wallet {

namespace {

constexpr int kCardanoKnapsackSolverIterations = 1000;

bool CheckBalance(const std::vector<CardanoTransaction::TxInput>& inputs,
                  const TxBuilderParms& builder_params) {
  base::CheckedNumeric<uint64_t> total_amount = 0;
  for (const auto& input : inputs) {
    if (!builder_params.token_to_send) {
      total_amount += input.utxo_value;
    } else {
      auto it = input.utxo_tokens.find(*builder_params.token_to_send);
      if (it != input.utxo_tokens.end()) {
        total_amount += it->second;
      }
    }
  }
  if (!total_amount.IsValid()) {
    return false;
  }
  return total_amount.ValueOrDie() >= builder_params.amount;
}

}  // namespace

CardanoKnapsackSolver::CardanoKnapsackSolver(
    TxBuilderParms builder_params,
    std::vector<CardanoTransaction::TxInput> inputs)
    : builder_params_(std::move(builder_params)), inputs_(std::move(inputs)) {}
CardanoKnapsackSolver::~CardanoKnapsackSolver() = default;

// static
base::expected<void, std::string> CardanoKnapsackSolver::SetupOutput(
    CardanoTransaction& tx,
    const TxBuilderParms& builder_params) {
  tx.SetupTargetOutput(builder_params.send_to_address);
  if (!builder_params.token_to_send) {
    tx.TargetOutput()->amount = builder_params.amount;
    if (!CardanoTransactionSerializer().ValidateMinValue(
            *tx.TargetOutput(), builder_params.epoch_parameters)) {
      return base::unexpected(WalletAmountTooSmallErrorMessage());
    }
  } else {
    tx.TargetOutput()->tokens[*builder_params.token_to_send] =
        builder_params.amount;
    auto min_ada_required_target =
        CardanoTransactionSerializer::CalcMinAdaRequired(
            *tx.TargetOutput(), builder_params.epoch_parameters);
    if (!base::OptionalUnwrapTo(min_ada_required_target,
                                tx.TargetOutput()->amount)) {
      return base::unexpected(WalletInsufficientBalanceErrorMessage());
    }
  }
  return base::ok();
}

// static
void CardanoKnapsackSolver::SortInputs(
    std::vector<CardanoTransaction::TxInput>& inputs,
    const TxBuilderParms& builder_params) {
  std::sort(
      inputs.begin(), inputs.end(),
      [&builder_params](const CardanoTransaction::TxInput& i1,
                        const CardanoTransaction::TxInput& i2) {
        if (builder_params.token_to_send) {
          const auto& token_id = *builder_params.token_to_send;
          auto it1 = i1.utxo_tokens.find(token_id);
          auto it2 = i2.utxo_tokens.find(token_id);
          uint64_t amount1 = it1 != i1.utxo_tokens.end() ? it1->second : 0;
          uint64_t amount2 = it2 != i2.utxo_tokens.end() ? it2->second : 0;
          if (amount1 != amount2) {
            return amount1 > amount2;
          }
        }
        return i1.utxo_value > i2.utxo_value;
      });
}

void CardanoKnapsackSolver::RunSolverForTransaction(
    const CardanoTransaction& transaction,
    std::optional<CardanoTransaction>& current_best_solution) {
  if (inputs_.empty()) {
    return;
  }

  std::vector<uint8_t> picked_inputs(inputs_.size(), false);
  for (int i = 0; i < kCardanoKnapsackSolverIterations; ++i) {
    std::ranges::fill(picked_inputs, false);

    CardanoTransaction cur_transaction = transaction;

    bool has_valid_transaction_for_iteration = false;

    // First pass: Go through inputs(sorted desc) and randomly pick
    // them. If we have a valid transaction discard last input and continue
    // trying to make valid transactions with smaller groups.
    // Second pass(if no valid transactions from 1st pass): Forcedly pick
    // yet not picked inputs(starting from largest).
    for (int pass = 0; pass < 2; ++pass) {
      if (has_valid_transaction_for_iteration) {
        DCHECK_EQ(pass, 1);
        break;
      }

      for (uint32_t input_index = 0; input_index < inputs_.size();
           ++input_index) {
        bool pick_input =
            pass == 0 ? base::RandInt(0, 1) : !picked_inputs[input_index];
        if (!pick_input) {
          continue;
        }

        CardanoTransaction try_transaction = cur_transaction;
        try_transaction.AddInput(inputs_[input_index]);

        // We must move all tokens from inputs into change output.
        if (try_transaction.EnsureTokensInChangeOutput()) {
          // Given set of inputs and outputs try to find valid outputs' amounts
          // and tx fee.
          if (auto found_valid_tx =
                  CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
                      std::move(try_transaction),
                      builder_params_.epoch_parameters, false)) {
            // Found a valid tx with given inputs. Check if it is the best
            // tx so far and don't update `cur_transaction` as it doesn't make
            // sense to add new inputs into it.
            has_valid_transaction_for_iteration = true;
            if (!current_best_solution ||
                current_best_solution->fee() > found_valid_tx->fee()) {
              current_best_solution = std::move(*found_valid_tx);
            }

            continue;
          }
        }

        // Could not make a valid tx with given inputs. Proceed to the next
        // iteration with current input added to `cur_transaction`.
        picked_inputs[input_index] = true;
        cur_transaction.AddInput(inputs_[input_index]);
      }
    }
  }
}

base::expected<CardanoTransaction, std::string> CardanoKnapsackSolver::Solve() {
  CHECK(!builder_params_.sending_max_amount);

  // Fail fast if we don't have enough balance(lovelace or token) to send to
  // destination address.
  if (!CheckBalance(inputs_, builder_params_)) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  CardanoTransaction base_transaction;
  base_transaction.set_invalid_after(builder_params_.invalid_after);
  if (auto err = SetupOutput(base_transaction, builder_params_);
      !err.has_value()) {
    return base::unexpected(err.error());
  }

  SortInputs(inputs_, builder_params_);

  std::optional<CardanoTransaction> current_best_solution;

  // Try to find the best transaction with a change output which receives a
  // fee surplus.
  auto tx_with_change = base_transaction;
  tx_with_change.SetupChangeOutput(builder_params_.change_address);
  RunSolverForTransaction(tx_with_change, current_best_solution);

  // Drop the change output from the transaction and try to find the best
  // transaction again. Might find a transaction with a slightly higher fee
  // but still less than the cost of having a change output.
  auto tx_no_change = base_transaction;
  RunSolverForTransaction(tx_no_change, current_best_solution);

  if (!current_best_solution) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  DCHECK_GT(current_best_solution->fee(), 0u);
  DCHECK(current_best_solution->witnesses().empty());
  return base::ok(std::move(*current_best_solution));
}

}  // namespace brave_wallet
