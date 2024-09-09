/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement_util.h"

#include <iterator>

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/statement/ads_received_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/ads_summary_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/earnings_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/next_payment_date_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

TransactionList FilterTransactionsForEstimatedEarnings(
    const TransactionList& transactions) {
  TransactionList filtered_transactions;
  base::ranges::copy_if(transactions, std::back_inserter(filtered_transactions),
                        [](const TransactionInfo& transaction) {
                          return transaction.ad_type !=
                                 mojom::AdType::kNewTabPageAd;
                        });
  return filtered_transactions;
}

}  // namespace

base::Time GetNextPaymentDate(const TransactionList& transactions) {
  const base::Time next_payment_token_redemption_at =
      GetProfileTimePref(prefs::kNextPaymentTokenRedemptionAt);

  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_payment_token_redemption_at, transactions);

  return next_payment_date;
}

std::pair<double, double> GetEstimatedEarningsForThisMonth(
    const TransactionList& transactions) {
  TransactionList filtered_transactions =
      FilterTransactionsForEstimatedEarnings(transactions);

  const double range_low =
      GetUnreconciledEarnings(filtered_transactions) +
      GetReconciledEarningsForThisMonth(filtered_transactions);

  const double range_high = GetUnreconciledEarnings(transactions) +
                            GetReconciledEarningsForThisMonth(transactions);

  return {range_low * kMinEstimatedEarningsMultiplier.Get(), range_high};
}

std::pair<double, double> GetEstimatedEarningsForPreviousMonth(
    const TransactionList& transactions) {
  const double range_low = GetReconciledEarningsForPreviousMonth(
      FilterTransactionsForEstimatedEarnings(transactions));
  const double range_high = GetReconciledEarningsForPreviousMonth(transactions);

  return {range_low * kMinEstimatedEarningsMultiplier.Get(), range_high};
}

int32_t GetAdsReceivedThisMonth(const TransactionList& transactions) {
  const base::Time from_time = LocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = LocalTimeAtEndOfThisMonth();

  return static_cast<int32_t>(
      GetAdsReceivedForDateRange(transactions, from_time, to_time));
}

base::flat_map<mojom::AdType, int32_t> GetAdsSummaryThisMonth(
    const TransactionList& transactions) {
  const base::Time from_time = LocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = LocalTimeAtEndOfThisMonth();

  return GetAdsSummaryForDateRange(transactions, from_time, to_time);
}

}  // namespace brave_ads
