/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_send_solver.h"

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

namespace brave_wallet {

namespace {

// If any of inputs have tokens attached to corresponding utxos make sure there
// is a valid change output having these tokens set to it.
bool MaybeSetupChangeOutputForTokens(
    CardanoTransaction& tx,
    const CardanoAddress& change_address,
    const cardano_rpc::EpochParameters& latest_epoch_parameters) {
  auto tokens = tx.GetTotalInputTokensAmount();
  if (!tokens) {
    return false;
  }
  if (tokens->empty()) {
    return true;
  }

  tx.SetupChangeOutput(change_address);
  if (!tx.EnsureTokensInChangeOutput()) {
    return false;
  }

  // Adjust change output amount so it can cover size of output increased by
  // tokens.
  auto min_ada_required = CardanoTransactionSerializer::CalcMinAdaRequired(
      *tx.ChangeOutput(), latest_epoch_parameters);
  if (!min_ada_required.has_value()) {
    return false;
  }
  tx.ChangeOutput()->amount = min_ada_required.value();

  return true;
}

}  // namespace

CardanoMaxSendSolver::CardanoMaxSendSolver(
    CardanoTransaction base_transaction,
    CardanoAddress change_address,
    cardano_rpc::EpochParameters latest_epoch_parameters,
    std::vector<CardanoTransaction::TxInput> inputs)
    : base_transaction_(std::move(base_transaction)),
      change_address_(std::move(change_address)),
      latest_epoch_parameters_(std::move(latest_epoch_parameters)),
      inputs_(std::move(inputs)) {}
CardanoMaxSendSolver::~CardanoMaxSendSolver() = default;

base::expected<CardanoTransaction, std::string> CardanoMaxSendSolver::Solve() {
  DCHECK_EQ(base_transaction_.inputs().size(), 0u);
  DCHECK(base_transaction_.TargetOutput());
  DCHECK(!base_transaction_.ChangeOutput());
  DCHECK(base_transaction_.sending_max_amount());

  auto tx = base_transaction_;
  tx.AddInputs(inputs_);

  if (!MaybeSetupChangeOutputForTokens(tx, change_address_,
                                       latest_epoch_parameters_)) {
    return base::unexpected(WalletInternalErrorMessage());
  }

  auto found_valid_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
      tx, latest_epoch_parameters_);
  if (!found_valid_tx) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  CHECK(CardanoTransactionSerializer::ValidateAmounts(
      *found_valid_tx, latest_epoch_parameters_));

  found_valid_tx->set_amount(found_valid_tx->TargetOutput()->amount);

  return base::ok(*found_valid_tx);
}

}  // namespace brave_wallet
