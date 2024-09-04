/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/ads_received_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

size_t GetAdsReceivedForDateRange(const TransactionList& transactions,
                                  const base::Time from_time,
                                  const base::Time to_time) {
  return base::ranges::count_if(
      transactions, [from_time, to_time](const TransactionInfo& transaction) {
        return transaction.confirmation_type ==
                   mojom::ConfirmationType::kViewedImpression &&
               transaction.created_at >= from_time &&
               transaction.created_at <= to_time;
      });
}

}  // namespace brave_ads
