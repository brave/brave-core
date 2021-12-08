/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/ads_received_util.h"

#include <algorithm>

#include "base/time/time.h"

namespace ads {

int GetAdsReceivedForDateRange(const TransactionList& transactions,
                               const base::Time& from_time,
                               const base::Time& to_time) {
  const double from_timestamp = from_time.ToDoubleT();
  const double to_timestamp = to_time.ToDoubleT();

  return std::count_if(
      transactions.cbegin(), transactions.cend(),
      [from_timestamp, to_timestamp](const TransactionInfo& transaction) {
        return transaction.confirmation_type == ConfirmationType::kViewed &&
               transaction.created_at >= from_timestamp &&
               transaction.created_at <= to_timestamp;
      });
}

}  // namespace ads
