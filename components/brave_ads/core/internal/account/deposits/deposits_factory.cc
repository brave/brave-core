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
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

std::unique_ptr<DepositInterface> DepositsFactory::Build(
    mojom::ConfirmationType mojom_confirmation_type) {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so all desposits are non-rewardable.
    return std::make_unique<NonCashDeposit>();
  }

  switch (mojom_confirmation_type) {
    case mojom::ConfirmationType::kViewedImpression: {
      // Rewardable.
      return std::make_unique<CashDeposit>();
    }

    case mojom::ConfirmationType::kClicked:
    case mojom::ConfirmationType::kDismissed:
    case mojom::ConfirmationType::kServedImpression:
    case mojom::ConfirmationType::kLanded:
    case mojom::ConfirmationType::kSavedAd:
    case mojom::ConfirmationType::kMarkAdAsInappropriate:
    case mojom::ConfirmationType::kLikedAd:
    case mojom::ConfirmationType::kDislikedAd:
    case mojom::ConfirmationType::kConversion:
    case mojom::ConfirmationType::kMediaPlay:
    case mojom::ConfirmationType::kMedia25:
    case mojom::ConfirmationType::kMedia100: {
      // Non-rewardable.
      return std::make_unique<NonCashDeposit>();
    }

    case mojom::ConfirmationType::kUndefined: {
      break;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::ConfirmationType: "
               << base::to_underlying(mojom_confirmation_type);
}

}  // namespace brave_ads
