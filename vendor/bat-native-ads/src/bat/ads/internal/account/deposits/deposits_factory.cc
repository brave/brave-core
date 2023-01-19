/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposits_factory.h"

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/deposits/cash_deposit.h"
#include "bat/ads/internal/account/deposits/non_cash_deposit.h"

namespace ads {

std::unique_ptr<DepositInterface> DepositsFactory::Build(
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) {
  if (ad_type == AdType::kNewTabPageAd && !ShouldRewardUser()) {
    return nullptr;
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
      return nullptr;
    }
  }
}

}  // namespace ads
