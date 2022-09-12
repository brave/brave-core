/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/contribution_monthly_util.h"

namespace ledger {
namespace contribution {

double GetTotalFromVerifiedTips(
    const std::vector<mojom::PublisherInfoPtr>& publisher_list) {
  double total_amount = 0.0;
  for (const auto& publisher : publisher_list) {
    if (!publisher || publisher->id.empty()) {
      continue;
    }

    if (publisher->status == mojom::PublisherStatus::UPHOLD_VERIFIED ||
        publisher->status == mojom::PublisherStatus::BITFLYER_VERIFIED) {
      total_amount += publisher->weight;
    }
  }

  return total_amount;
}

}  // namespace contribution
}  // namespace ledger
