/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/next_payment_date_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"

namespace brave_ads {

base::Time CalculateNextPaymentDate(
    const base::Time next_payment_token_redemption_at,
    const TransactionList& transactions) {
  const base::Time now = base::Time::Now();

  base::Time::Exploded now_exploded;
  now.UTCExplode(&now_exploded);

  int month = now_exploded.month;

  if (now_exploded.day_of_month <= kNextPaymentDay.Get()) {
    // Today is on or before our next payment day
    if (DidReconcileTransactionsPreviousMonth(transactions)) {
      // If last month has reconciled transactions, then the next payment date
      // will occur this month
    } else {
      // If last month does not have reconciled transactions, then the next
      // payment date will occur next month
      ++month;
    }
  } else {
    // Today is after our next payment day
    if (DidReconcileTransactionsThisMonth(transactions)) {
      // If this month has reconciled transactions, then the next payment date
      // will occur next month
      ++month;
    } else {
      base::Time::Exploded next_payment_token_redemption_at_exploded;
      next_payment_token_redemption_at.UTCExplode(
          &next_payment_token_redemption_at_exploded);

      if (next_payment_token_redemption_at_exploded.month == month) {
        // If this month does not have reconciled transactions and our next
        // token redemption date is this month, then the next payment date will
        // occur next month
        ++month;
      } else {
        // If this month does not have reconciled transactions and our next
        // token redemption date is next month, then the next payment date will
        // occur the month after next
        month += 2;
      }
    }
  }

  int year = now_exploded.year;

  if (month > 12) {
    month -= 12;
    ++year;
  }

  base::Time::Exploded next_payment_date_exploded = now_exploded;
  next_payment_date_exploded.year = year;
  next_payment_date_exploded.month = month;
  next_payment_date_exploded.day_of_month = kNextPaymentDay.Get();
  next_payment_date_exploded.hour = 23;
  next_payment_date_exploded.minute = 59;
  next_payment_date_exploded.second = 59;
  next_payment_date_exploded.millisecond = 999;

  base::Time next_payment_date;
  CHECK(base::Time::FromUTCExploded(next_payment_date_exploded,
                                    &next_payment_date));

  return next_payment_date;
}

}  // namespace brave_ads
