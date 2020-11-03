/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement.h"

#include "bat/ads/internal/account/transactions.h"
#include "bat/ads/internal/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/statement_info.h"

namespace ads {

Statement::Statement(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Statement::~Statement() = default;

StatementInfo Statement::Get(
    const int64_t from_timestamp,
    const int64_t to_timestamp) {
  DCHECK(to_timestamp >= from_timestamp);

  StatementInfo statement;

  statement.estimated_pending_rewards =
      ads_->get_ad_rewards()->GetEstimatedPendingRewards();

  statement.next_payment_date_in_seconds =
      ads_->get_ad_rewards()->GetNextPaymentDateInSeconds();

  statement.ad_notifications_received_this_month =
      ads_->get_ad_rewards()->GetAdNotificationsReceivedThisMonth();

  Transactions transactions(ads_);
  statement.transactions =
      transactions.Get(from_timestamp, to_timestamp);

  return statement;
}

}  // namespace ads
