/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_lovelace_send_solver.h"

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

namespace brave_wallet {

CardanoMaxLovelaceSendSolver::CardanoMaxLovelaceSendSolver(
    TxBuilderParms builder_params,
    std::vector<CardanoTransaction::TxInput> inputs)
    : builder_params_(std::move(builder_params)), inputs_(std::move(inputs)) {}
CardanoMaxLovelaceSendSolver::~CardanoMaxLovelaceSendSolver() = default;

// static
bool CardanoMaxLovelaceSendSolver::SetupOutputs(
    CardanoTransaction& tx,
    const TxBuilderParms& builder_params) {
  tx.SetupTargetOutput(builder_params.send_to_address);

  auto tokens = tx.GetTotalInputTokensAmount();
  if (!tokens) {
    return false;
  }
  if (tokens->empty()) {
    return true;
  }

  tx.SetupChangeOutput(builder_params.change_address);
  if (!tx.EnsureTokensInChangeOutput()) {
    return false;
  }

  // Adjust change output amount so it can cover size of output increased by
  // tokens.
  auto min_ada_required = CardanoTransactionSerializer::CalcMinAdaRequired(
      *tx.ChangeOutput(), builder_params.epoch_parameters);
  if (!min_ada_required.has_value()) {
    return false;
  }
  tx.ChangeOutput()->amount = min_ada_required.value();

  return true;
}

base::expected<CardanoTransaction, std::string>
CardanoMaxLovelaceSendSolver::Solve() {
  CHECK(builder_params_.sending_max_amount);
  CHECK_EQ(builder_params_.amount, 0u);
  CHECK(!builder_params_.token_to_send);

  CardanoTransaction tx;
  tx.set_invalid_after(builder_params_.invalid_after);
  tx.AddInputs(inputs_);

  if (!SetupOutputs(tx, builder_params_)) {
    return base::unexpected(WalletInternalErrorMessage());
  }

  auto found_valid_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
      std::move(tx), builder_params_.epoch_parameters, true);
  if (!found_valid_tx) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  CHECK(CardanoTransactionSerializer::ValidateAmounts(
      *found_valid_tx, builder_params_.epoch_parameters));

  return base::ok(*found_valid_tx);
}

}  // namespace brave_wallet
