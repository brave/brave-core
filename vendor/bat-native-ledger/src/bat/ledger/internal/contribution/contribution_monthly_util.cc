/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/contribution_monthly_util.h"

namespace braveledger_contribution {

double GetTotalFromVerifiedTips(
    const ledger::PublisherInfoList& publisher_list) {
  double total_amount = 0.0;
  for (const auto& publisher : publisher_list) {
    if (!publisher || publisher->id.empty()) {
      continue;
    }

    if (publisher->status == ledger::PublisherStatus::VERIFIED) {
      total_amount += publisher->weight;
    }
  }

  return total_amount;
}

}  // namespace braveledger_contribution
