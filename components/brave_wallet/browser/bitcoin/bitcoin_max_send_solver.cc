/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_max_send_solver.h"

#include <utility>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

BitcoinMaxSendSolver::BitcoinMaxSendSolver(
    BitcoinTransaction base_transaction,
    double fee_rate,
    const std::vector<BitcoinTransaction::TxInputGroup>& input_groups)
    : base_transaction_(std::move(base_transaction)),
      fee_rate_(fee_rate),
      input_groups_(input_groups) {}
BitcoinMaxSendSolver::~BitcoinMaxSendSolver() = default;

base::expected<BitcoinTransaction, std::string> BitcoinMaxSendSolver::Solve() {
  DCHECK_EQ(base_transaction_.inputs().size(), 0u);
  DCHECK(base_transaction_.TargetOutput());
  DCHECK_EQ(base_transaction_.TargetOutput()->amount, 0u);
  DCHECK(!base_transaction_.ChangeOutput());

  auto result = base_transaction_;
  for (auto& group : input_groups_) {
    // TODO(apaymyshev): avoid dust inputs?
    result.AddInputs(group.inputs());
  }

  uint64_t min_fee = ApplyFeeRate(
      fee_rate_, BitcoinSerializer::CalcTransactionVBytes(result, true));
  if (result.TotalInputsAmount() <= min_fee) {
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE));
  }

  result.TargetOutput()->amount = result.TotalInputsAmount() - min_fee;
  result.set_amount(result.TargetOutput()->amount);

  return base::ok(result);
}

}  // namespace brave_wallet
