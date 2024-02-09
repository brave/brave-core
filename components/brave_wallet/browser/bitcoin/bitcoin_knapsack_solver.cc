/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_knapsack_solver.h"

#include <algorithm>
#include <utility>

#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr int kKnapsackSolverIterations = 1000;

}  // namespace

KnapsackSolver::KnapsackSolver(
    BitcoinTransaction base_transaction,
    double fee_rate,
    double longterm_fee_rate,
    const std::vector<BitcoinTransaction::TxInputGroup>& input_groups)
    : base_transaction_(std::move(base_transaction)),
      fee_rate_(fee_rate),
      longterm_fee_rate_(longterm_fee_rate),
      input_groups_(input_groups) {}
KnapsackSolver::~KnapsackSolver() = default;

// static
uint64_t KnapsackSolver::GetCostOfChangeOutput(
    const BitcoinTransaction::TxOutput& output,
    double fee_rate,
    double longterm_fee_rate) {
  uint32_t output_vbytes_size =
      BitcoinSerializer::CalcOutputVBytesInTransaction(output);

  BitcoinTransaction::TxInput input;
  input.utxo_address = output.address;
  uint32_t input_vbytes_size =
      BitcoinSerializer::CalcInputVBytesInTransaction(input);

  // Having change output in transaction has some cost now(based on `fee_rate`)
  // and will cost more when used as an input(based on `longterm_fee_rate`)
  return ApplyFeeRate(fee_rate, output_vbytes_size) +
         ApplyFeeRate(longterm_fee_rate, input_vbytes_size);
}

void KnapsackSolver::SolveForTransaction(
    const BitcoinTransaction& transaction,
    std::multimap<uint64_t, BitcoinTransaction>& solutions) {
  // Don't create transaction if output's amount appears to be less than this
  // threshold. Cost of spending such output would be higher than its value.
  uint64_t dust_output_threshold =
      transaction.ChangeOutput()
          ? GetCostOfChangeOutput(*transaction.ChangeOutput(), fee_rate_,
                                  longterm_fee_rate_)
          : 0;

  for (int i = 0; i < kKnapsackSolverIterations; ++i) {
    std::vector<bool> picked_groups(input_groups_.size(), false);
    BitcoinTransaction cur_transaction = transaction;
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

      for (uint32_t group_index = 0; group_index < input_groups_.size();
           ++group_index) {
        bool pick_group =
            pass == 0 ? base::RandInt(0, 1) : !picked_groups[group_index];
        if (!pick_group) {
          continue;
        }

        // TODO(apaymyshev): avoid copying transaction. Just keep track of
        // current vbytes of transaction and optimize by cost of adding an
        // input.
        BitcoinTransaction next_transaction = cur_transaction;
        next_transaction.AddInputs(input_groups_[group_index].inputs());

        // Minimum fee required for this transaction to be accepted.
        // Depends on transaction's size and current fee rate.
        uint64_t min_fee = ApplyFeeRate(
            fee_rate_,
            BitcoinSerializer::CalcTransactionVBytes(next_transaction, true));

        // Move everything except `min_fee` to change output(if any). Throw away
        // possible transaction if resulting change amount is less than dust
        // threshold.
        if (auto change_amount =
                next_transaction.MoveSurplusFeeToChangeOutput(min_fee)) {
          if (change_amount < dust_output_threshold) {
            continue;
          }
        }

        if (next_transaction.AmountsAreValid(min_fee)) {
          has_valid_transaction_for_iteration = true;
          // TODO(apaymyshev): Should we also add cost of spending change output
          // in the future?
          solutions.emplace(next_transaction.EffectiveFeeAmount(),
                            next_transaction);

          // Keep some best ones(less fee) in the container. Might be useful for
          // logging later.
          while (solutions.size() > 10) {
            solutions.erase(std::prev(solutions.end()));
          }
        } else {
          picked_groups[group_index] = true;
          cur_transaction = std::move(next_transaction);
        }
      }
    }
  }
}

base::expected<BitcoinTransaction, std::string> KnapsackSolver::Solve() {
  DCHECK_EQ(base_transaction_.inputs().size(), 0u);
  DCHECK(base_transaction_.TargetOutput());
  DCHECK(base_transaction_.ChangeOutput());
  DCHECK(!base_transaction_.sending_max_amount());

  // TODO(apaymyshev): avoid dust inputs?
  std::sort(input_groups_.begin(), input_groups_.end(), [](auto& g1, auto& g2) {
    return g1.total_amount() > g2.total_amount();
  });

  std::multimap<uint64_t, BitcoinTransaction> solutions;

  // Try to find the best transaction with a change output which receives a fee
  // surplus.
  SolveForTransaction(base_transaction_, solutions);

  // Drop the change output from the transaction and try to find the best
  // transaction again. Might find a transaction with a slightly higher fee but
  // still less than the cost of having a change output.
  auto no_change_transaction = base_transaction_;
  no_change_transaction.ClearChangeOutput();
  SolveForTransaction(no_change_transaction, solutions);

  if (solutions.empty()) {
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE));
  }

  return base::ok(std::move(solutions.begin()->second));
}

}  // namespace brave_wallet
