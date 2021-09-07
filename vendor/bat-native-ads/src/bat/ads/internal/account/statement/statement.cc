/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include "base/check.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/statement_info.h"

namespace ads {

Statement::Statement(AdRewards* ad_rewards) : ad_rewards_(ad_rewards) {
  DCHECK(ad_rewards_);
}

Statement::~Statement() = default;

StatementInfo Statement::Get(const int64_t from_timestamp,
                             const int64_t to_timestamp) const {
  DCHECK(to_timestamp >= from_timestamp);

  StatementInfo statement;

  statement.next_payment_date = ad_rewards_->GetNextPaymentDate();

  statement.ads_received_this_month = GetAdsReceivedThisMonth();

  statement.earnings_this_month = GetEarningsForThisMonth();

  statement.earnings_last_month = GetEarningsForLastMonth();

  statement.cleared_transactions =
      transactions::GetCleared(from_timestamp, to_timestamp);

  statement.uncleared_transactions = transactions::GetUncleared();

  return statement;
}

///////////////////////////////////////////////////////////////////////////////

double Statement::GetEarningsForThisMonth() const {
  return ad_rewards_->GetEarningsForThisMonth();
}

double Statement::GetEarningsForLastMonth() const {
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.UTCExplode(&exploded);

  exploded.month--;
  if (exploded.month < 1) {
    exploded.month = 12;
    exploded.year--;
  }

  exploded.day_of_month = 1;

  base::Time last_month;
  const bool success = base::Time::FromUTCExploded(exploded, &last_month);
  DCHECK(success);

  return ad_rewards_->GetEarningsForMonth(last_month);
}

uint64_t Statement::GetAdsReceivedThisMonth() const {
  const base::Time now = base::Time::Now();
  return transactions::GetCountForMonth(now);
}

}  // namespace ads
