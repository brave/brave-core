/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/statement/ads_received_util.h"
#include "bat/ads/internal/account/statement/earnings_util.h"
#include "bat/ads/internal/account/statement/next_payment_date_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/time/time_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

base::Time GetNextPaymentDate(const TransactionList& transactions) {
  const base::Time next_token_redemption_at =
      AdsClientHelper::GetInstance()->GetTimePref(
          prefs::kNextTokenRedemptionAt);

  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  return next_payment_date;
}

double GetEarningsForThisMonth(const TransactionList& transactions) {
  const double unreconciled_earnings = GetUnreconciledEarnings(transactions);

  const double reconciled_earnings_this_month =
      GetReconciledEarningsForThisMonth(transactions);

  return unreconciled_earnings + reconciled_earnings_this_month;
}

double GetEarningsForLastMonth(const TransactionList& transactions) {
  return GetReconciledEarningsForLastMonth(transactions);
}

int GetAdsReceivedThisMonth(const TransactionList& transactions) {
  const base::Time from_time = GetLocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  return GetAdsReceivedForDateRange(transactions, from_time, to_time);
}

}  // namespace ads
