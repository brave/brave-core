/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement_util.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/account_feature.h"
#include "brave/components/brave_ads/core/internal/account/statement/ads_received_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/earnings_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/next_payment_date_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

namespace brave_ads {

base::Time GetNextPaymentDate(const TransactionList& transactions) {
  const base::Time next_token_redemption_at =
      AdsClientHelper::GetInstance()->GetTimePref(
          prefs::kNextTokenRedemptionAt);

  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  return next_payment_date;
}

std::pair<double, double> GetEstimatedEarningsForThisMonth(
    const TransactionList& transactions) {
  TransactionList filtered_transactions;
  base::ranges::copy_if(transactions, std::back_inserter(filtered_transactions),
                        [](const TransactionInfo& transaction) {
                          return transaction.ad_type != AdType::kNewTabPageAd;
                        });

  const double range_low =
      GetUnreconciledEarnings(filtered_transactions) +
      GetReconciledEarningsForThisMonth(filtered_transactions);

  const double range_high = GetUnreconciledEarnings(transactions) +
                            GetReconciledEarningsForThisMonth(transactions);

  return {range_low * kMinEstimatedEarningsMultiplier.Get(), range_high};
}

double GetEarningsForLastMonth(const TransactionList& transactions) {
  return GetReconciledEarningsForLastMonth(transactions);
}

int32_t GetAdsReceivedThisMonth(const TransactionList& transactions) {
  const base::Time from_time = GetLocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  return static_cast<int32_t>(
      GetAdsReceivedForDateRange(transactions, from_time, to_time));
}

}  // namespace brave_ads
