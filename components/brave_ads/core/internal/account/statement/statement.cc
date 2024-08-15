/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

void BuildStatement(BuildStatementCallback callback) {
  GetTransactionsForDateRange(
      /*from_time=*/base::Time(), /*to_time=*/LocalTimeAtEndOfThisMonth(),
      base::BindOnce(
          [](BuildStatementCallback callback, const bool success,
             const TransactionList& transactions) {
            if (!success) {
              BLOG(0, "Failed to get transactions");

              return std::move(callback).Run(/*statement=*/nullptr);
            }

            mojom::StatementInfoPtr mojom_statement =
                mojom::StatementInfo::New();

            const auto [min_earnings_previous_month,
                        max_earnings_previous_month] =
                GetEstimatedEarningsForPreviousMonth(transactions);
            mojom_statement->min_earnings_previous_month =
                min_earnings_previous_month;
            mojom_statement->max_earnings_previous_month =
                max_earnings_previous_month;

            const auto [min_earnings_this_month, max_earnings_this_month] =
                GetEstimatedEarningsForThisMonth(transactions);
            mojom_statement->min_earnings_this_month = min_earnings_this_month;
            mojom_statement->max_earnings_this_month = max_earnings_this_month;

            mojom_statement->next_payment_date =
                GetNextPaymentDate(transactions);

            mojom_statement->ads_received_this_month =
                GetAdsReceivedThisMonth(transactions);

            mojom_statement->ads_summary_this_month =
                GetAdsSummaryThisMonth(transactions);

            std::move(callback).Run(std::move(mojom_statement));
          },
          std::move(callback)));
}

}  // namespace brave_ads
