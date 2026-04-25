/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_token_send_solver.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

namespace brave_wallet {

// Setup outputs for tx to send all inputs with a given token.
// Target output gets all amount of given token and some min lovelace to cover
// fee.
// Change output gets everything else.
// static
bool CardanoMaxTokenSendSolver::SetupOutputs(
    CardanoTransaction& tx,
    const TxBuilderParms& builder_params) {
  auto tokens = tx.GetTotalInputTokensAmount();
  if (!tokens) {
    return false;
  }

  auto token_value = tokens->find(builder_params.token_to_send.value());
  if (token_value == tokens->end()) {
    return false;
  }

  auto token_sum = token_value->second;
  CHECK_GT(token_sum, 0u);
  tx.SetupTargetOutput(builder_params.send_to_address);
  tx.TargetOutput()->tokens[builder_params.token_to_send.value()] = token_sum;

  auto min_ada_required_target =
      CardanoTransactionSerializer::CalcMinAdaRequired(
          *tx.TargetOutput(), builder_params.epoch_parameters);
  if (!min_ada_required_target.has_value()) {
    return false;
  }
  tx.TargetOutput()->amount = min_ada_required_target.value();

  tx.SetupChangeOutput(builder_params.change_address);
  if (!tx.EnsureTokensInChangeOutput()) {
    return false;
  }

  return true;
}

// static
std::pair<std::vector<CardanoTransaction::TxInput>,
          std::vector<CardanoTransaction::TxInput>>
CardanoMaxTokenSendSolver::SplitInputsByToken(
    const cardano_rpc::TokenId& token_id,
    std::vector<CardanoTransaction::TxInput> inputs) {
  std::vector<CardanoTransaction::TxInput> token_inputs;
  std::vector<CardanoTransaction::TxInput> other_inputs;
  for (auto& input : inputs) {
    if (input.utxo_tokens.contains(token_id)) {
      token_inputs.push_back(std::move(input));
    } else {
      other_inputs.push_back(std::move(input));
    }
  }

  return {std::move(token_inputs), std::move(other_inputs)};
}

// static
void CardanoMaxTokenSendSolver::SortInputsBySelectionPriority(
    std::vector<CardanoTransaction::TxInput>& inputs) {
  std::sort(
      inputs.begin(), inputs.end(),
      [](CardanoTransaction::TxInput& i1, CardanoTransaction::TxInput& i2) {
        if (i1.utxo_tokens.size() != i2.utxo_tokens.size()) {
          return i1.utxo_tokens.size() < i2.utxo_tokens.size();
        }
        return i1.utxo_value > i2.utxo_value;
      });
}

CardanoMaxTokenSendSolver::CardanoMaxTokenSendSolver(
    TxBuilderParms builder_params,
    std::vector<CardanoTransaction::TxInput> inputs)
    : builder_params_(std::move(builder_params)), inputs_(std::move(inputs)) {}
CardanoMaxTokenSendSolver::~CardanoMaxTokenSendSolver() = default;

base::expected<CardanoTransaction, std::string>
CardanoMaxTokenSendSolver::Solve() {
  CHECK(builder_params_.sending_max_amount);
  CHECK_EQ(builder_params_.amount, 0u);
  CHECK(builder_params_.token_to_send);

  auto [token_inputs, other_inputs] =
      SplitInputsByToken(*builder_params_.token_to_send, std::move(inputs_));

  if (token_inputs.empty()) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  SortInputsBySelectionPriority(other_inputs);
  auto other_inputs_span = base::span(other_inputs);

  std::vector<CardanoTransaction::TxInput> cur_inputs;

  while (!(token_inputs.empty() && other_inputs_span.empty())) {
    if (!token_inputs.empty()) {
      // At the first run try only inputs with tokens.
      cur_inputs = std::move(token_inputs);
    } else {
      // For the next runs try to pick from other inputs to cover fee.
      cur_inputs.push_back(other_inputs_span.take_first_elem());
    }

    CardanoTransaction tx;
    tx.set_invalid_after(builder_params_.invalid_after);
    tx.AddInputs(cur_inputs);

    if (!SetupOutputs(tx, builder_params_)) {
      return base::unexpected(WalletInternalErrorMessage());
    }

    if (auto found_valid_tx =
            CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
                std::move(tx), builder_params_.epoch_parameters, false)) {
      return base::ok(*found_valid_tx);
    }
  }

  return base::unexpected(WalletInsufficientBalanceErrorMessage());
}

}  // namespace brave_wallet
