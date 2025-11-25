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

  auto result = base_transaction_;
  result.AddInputs(std::move(inputs_));

  if (!CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
          result, latest_epoch_parameters_)) {
    return base::unexpected(WalletInsufficientBalanceErrorMessage());
  }

  CHECK(CardanoTransactionSerializer::ValidateAmounts(
      result, latest_epoch_parameters_));

  result.set_amount(result.TargetOutput()->amount);

  return base::ok(result);
}

}  // namespace brave_wallet
