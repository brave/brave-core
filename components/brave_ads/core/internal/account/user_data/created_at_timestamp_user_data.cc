/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/created_at_timestamp_user_data.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

namespace brave_ads {

namespace {
constexpr char kCreatedAtTimestampKey[] = "createdAtTimestamp";
}  // namespace

base::Value::Dict BuildCreatedAtTimestampUserData(
    const TransactionInfo& transaction) {
  base::Value::Dict user_data;

  user_data.Set(kCreatedAtTimestampKey,
                TimeToPrivacyPreservingISO8601(transaction.created_at));

  return user_data;
}

}  // namespace brave_ads
