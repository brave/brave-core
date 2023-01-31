/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/ads_received_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"

namespace ads {

int GetAdsReceivedForDateRange(const TransactionList& transactions,
                               const base::Time from_time,
                               const base::Time to_time) {
  return static_cast<int>(base::ranges::count_if(
      transactions, [from_time, to_time](const TransactionInfo& transaction) {
        return transaction.confirmation_type == ConfirmationType::kViewed &&
               transaction.created_at >= from_time &&
               transaction.created_at <= to_time;
      }));
}

}  // namespace ads
