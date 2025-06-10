/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_max_send_solver.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/numerics/clamped_math.h"
#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

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

  uint64_t fee = CardanoSerializer({.max_value_for_target_output = true,
                                    .use_dummy_witness_set = true})
                     .CalcMinTransactionFee(result, latest_epoch_parameters_);

  if (result.TotalInputsAmount() <= fee) {
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE));
  }

  result.TargetOutput()->amount = result.TotalInputsAmount() - fee;
  result.set_amount(result.TargetOutput()->amount);

  return base::ok(std::move(result));
}

}  // namespace brave_wallet
