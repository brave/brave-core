/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"

#include "base/notreached.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/deposits/cash_deposit.h"
#include "brave/components/brave_ads/core/internal/account/deposits/non_cash_deposit.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

std::unique_ptr<DepositInterface> DepositsFactory::Build(
    const ConfirmationType& confirmation_type) {
  if (!UserHasJoinedBraveRewards()) {
    return std::make_unique<NonCashDeposit>();
  }

  switch (confirmation_type.value()) {
    case ConfirmationType::kViewed: {
      return std::make_unique<CashDeposit>();
    }

    case ConfirmationType::kClicked:
    case ConfirmationType::kDismissed:
    case ConfirmationType::kServed:
    case ConfirmationType::kTransferred:
    case ConfirmationType::kSaved:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      return std::make_unique<NonCashDeposit>();
    }

    case ConfirmationType::kUndefined: {
      NOTREACHED_NORETURN();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << confirmation_type;
}

}  // namespace brave_ads
