/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include <utility>

#include "base/time/time.h"
#include "bat/ads/internal/account/statement/statement_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/time/time_util.h"

namespace ads {

void BuildStatement(BuildStatementCallback callback) {
  const base::Time from_time = GetTimeInDistantPast();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  transactions::GetForDateRange(
      from_time, to_time,
      [callback](const bool success, const TransactionList& transactions) {
        if (!success) {
          BLOG(0, "Failed to get transactions");
          callback(/* statement */ nullptr);
          return;
        }

        mojom::StatementInfoPtr statement = mojom::StatementInfo::New();
        statement->earnings_last_month = GetEarningsForLastMonth(transactions);
        statement->earnings_this_month = GetEarningsForThisMonth(transactions);
        statement->next_payment_date = GetNextPaymentDate(transactions);
        statement->ads_received_this_month =
            GetAdsReceivedThisMonth(transactions);

        callback(std::move(statement));
      });
}

}  // namespace ads
