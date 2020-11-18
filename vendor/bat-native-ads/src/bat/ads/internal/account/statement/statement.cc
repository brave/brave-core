/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/statement_info.h"

namespace ads {

Statement::Statement(
    AdRewards* ad_rewards)
    : ad_rewards_(ad_rewards) {
  DCHECK(ad_rewards_);
}

Statement::~Statement() = default;

StatementInfo Statement::Get(
    const int64_t from_timestamp,
    const int64_t to_timestamp) {
  DCHECK(to_timestamp >= from_timestamp);

  StatementInfo statement;

  statement.estimated_pending_rewards =
      ad_rewards_->GetEstimatedPendingRewards();

  statement.next_payment_date_in_seconds =
      ad_rewards_->GetNextPaymentDateInSeconds();

  statement.ad_notifications_received_this_month =
      ad_rewards_->GetAdNotificationsReceivedThisMonth();

  statement.transactions =
      transactions::GetCleared(from_timestamp, to_timestamp);

  statement.uncleared_transactions = transactions::GetUncleared();

  return statement;
}

}  // namespace ads
