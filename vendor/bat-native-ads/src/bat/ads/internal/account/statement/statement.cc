/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/statement/statement_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/statement_info.h"

namespace ads {

void BuildStatement(StatementCallback callback) {
  const base::Time& from_time = base::Time();
  const base::Time& to_time = base::Time::Now();

  transactions::GetForDateRange(
      from_time, to_time,
      [=](const bool success, const TransactionList& transactions) {
        if (!success) {
          BLOG(0, "Failed to get transactions");
          callback(/* success */ false, {});
          return;
        }

        StatementInfo statement;

        statement.earnings_this_month =
            GetEarningsForThisMonth(transactions) +
            GetUnreconciledEarningsForPreviousMonths(transactions);

        statement.earnings_last_month =
            GetReconciledEarningsForLastMonth(transactions);

        const base::Time& next_payment_date = GetNextPaymentDate(transactions);
        statement.next_payment_date = next_payment_date.ToDoubleT();

        statement.ads_received_this_month =
            GetAdsReceivedForThisMonth(transactions);

        callback(/* success */ true, statement);
      });
}

}  // namespace ads
