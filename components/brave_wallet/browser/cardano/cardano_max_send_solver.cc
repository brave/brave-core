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

CardanoMaxSendSolver::CardanoMaxSendSolver(
    CardanoTransaction base_transaction,
    cardano_rpc::EpochParameters latest_epoch_parameters,
    std::vector<CardanoTransaction::TxInput> inputs)
    : base_transaction_(std::move(base_transaction)),
      latest_epoch_parameters_(std::move(latest_epoch_parameters)),
      inputs_(std::move(inputs)) {}
CardanoMaxSendSolver::~CardanoMaxSendSolver() = default;

base::expected<CardanoTransaction, std::string> CardanoMaxSendSolver::Solve() {
  DCHECK_EQ(base_transaction_.inputs().size(), 0u);
  DCHECK(base_transaction_.TargetOutput());
  DCHECK(base_transaction_.sending_max_amount());

  auto tx_with_inputs = base_transaction_;
  tx_with_inputs.AddInputs(std::move(inputs_));

  auto found_valid_tx = CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
      tx_with_inputs, latest_epoch_parameters_);
  if (!found_valid_tx) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  CHECK(CardanoTransactionSerializer::ValidateAmounts(
      *found_valid_tx, latest_epoch_parameters_));

  found_valid_tx->set_amount(found_valid_tx->TargetOutput()->amount);

  return base::ok(*found_valid_tx);
}

}  // namespace brave_wallet
