/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/constants.h"

namespace ledger {
namespace contribution {

ledger::ReportType GetReportTypeFromRewardsType(
    const ledger::RewardsType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE): {
      return ledger::ReportType::AUTO_CONTRIBUTION;
    }
    case static_cast<int>(ledger::RewardsType::ONE_TIME_TIP): {
      return ledger::ReportType::TIP;
    }
    case static_cast<int>(ledger::RewardsType::RECURRING_TIP): {
      return ledger::ReportType::TIP_RECURRING;
    }
    default: {
      // missing conversion, returning dummy value.
      NOTREACHED();
      return ledger::ReportType::TIP;
    }
  }
}

ledger::ContributionProcessor GetProcessor(const std::string& wallet_type) {
  if (wallet_type == constant::kWalletUnBlinded) {
    return ledger::ContributionProcessor::BRAVE_TOKENS;
  }

  if (wallet_type == constant::kWalletAnonymous) {
    return ledger::ContributionProcessor::BRAVE_USER_FUNDS;
  }

  if (wallet_type == constant::kWalletUphold) {
    return ledger::ContributionProcessor::UPHOLD;
  }

  return ledger::ContributionProcessor::NONE;
}

std::string GetNextProcessor(const std::string& current_processor) {
  if (current_processor == constant::kWalletUnBlinded) {
    return constant::kWalletAnonymous;
  }

  if (current_processor == constant::kWalletAnonymous) {
    return constant::kWalletUphold;
  }

  if (current_processor == constant::kWalletUphold) {
    return "";
  }

  return constant::kWalletUnBlinded;
}

bool HaveEnoughFundsToContribute(
    double* amount,
    const bool partial,
    const double balance) {
  DCHECK(amount);

  if (partial) {
    if (balance == 0) {
      return false;
    }

    if (*amount > balance) {
      *amount = balance;
    }

    return true;
  }

  if (*amount > balance) {
    return false;
  }

  return true;
}

int32_t GetVotesFromAmount(const double amount) {
  DCHECK_GT(ledger::constant::kVotePrice, 0);
  return std::floor(amount / constant::kVotePrice);
}

}  // namespace contribution
}  // namespace ledger
