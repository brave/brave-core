/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_util.h"

namespace braveledger_contribution {

braveledger_bat_helper::Directions
FromContributionQueuePublishersToReconcileDirections(
    ledger::ContributionQueuePublisherList list) {
  braveledger_bat_helper::Directions directions;

  for (auto& item : list) {
    if (!item || item->publisher_key.empty()) {
      continue;
    }

    const auto direction = braveledger_bat_helper::RECONCILE_DIRECTION(
      item->publisher_key,
      item->amount_percent);

    directions.push_back(direction);
  }

  return directions;
}

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

}  // namespace braveledger_contribution
