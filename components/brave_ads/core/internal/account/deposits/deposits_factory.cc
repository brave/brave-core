/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/account/deposits/cash_deposit.h"
#include "brave/components/brave_ads/core/internal/account/deposits/non_cash_deposit.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

std::unique_ptr<DepositInterface> DepositsFactory::Build(
    const ConfirmationType confirmation_type) {
  if (!UserHasJoinedBraveRewards()) {
    return std::make_unique<NonCashDeposit>();
  }

  switch (confirmation_type) {
    case ConfirmationType::kViewedImpression: {
      return std::make_unique<CashDeposit>();
    }

    case ConfirmationType::kClicked:
    case ConfirmationType::kDismissed:
    case ConfirmationType::kServedImpression:
    case ConfirmationType::kLanded:
    case ConfirmationType::kSavedAd:
    case ConfirmationType::kMarkAdAsInappropriate:
    case ConfirmationType::kLikedAd:
    case ConfirmationType::kDislikedAd:
    case ConfirmationType::kConversion:
    case ConfirmationType::kMediaPlay:
    case ConfirmationType::kMedia25:
    case ConfirmationType::kMedia100: {
      return std::make_unique<NonCashDeposit>();
    }

    case ConfirmationType::kUndefined: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << base::to_underlying(confirmation_type);
}

}  // namespace brave_ads
