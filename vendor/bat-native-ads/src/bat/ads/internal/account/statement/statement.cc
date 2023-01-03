/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "bat/ads/internal/account/statement/statement_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

void BuildStatement(BuildStatementCallback callback) {
  const base::Time from_time = GetTimeInDistantPast();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  transactions::GetForDateRange(
      from_time, to_time,
      base::BindOnce(
          [](BuildStatementCallback callback, const bool success,
             const TransactionList& transactions) {
            if (!success) {
              BLOG(0, "Failed to get transactions");
              std::move(callback).Run(/*statement*/ nullptr);
              return;
            }

            mojom::StatementInfoPtr statement = mojom::StatementInfo::New();
            statement->earnings_last_month =
                GetEarningsForLastMonth(transactions);
            statement->earnings_this_month =
                GetEarningsForThisMonth(transactions);
            statement->next_payment_date = GetNextPaymentDate(transactions);
            statement->ads_received_this_month =
                GetAdsReceivedThisMonth(transactions);

            std::move(callback).Run(std::move(statement));
          },
          std::move(callback)));
}

}  // namespace ads
