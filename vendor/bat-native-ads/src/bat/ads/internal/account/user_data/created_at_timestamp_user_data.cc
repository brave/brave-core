/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/created_at_timestamp_user_data.h"

#include "base/time/time.h"
#include "bat/ads/internal/common/time/time_util.h"

namespace ads::user_data {

namespace {
constexpr char kCreatedAtTimestampKey[] = "createdAtTimestamp";
}  // namespace

base::Value::Dict GetCreatedAtTimestamp(const base::Time created_at) {
  base::Value::Dict user_data;

  user_data.Set(kCreatedAtTimestampKey,
                TimeToPrivacyPreservingISO8601(created_at));

  return user_data;
}

}  // namespace ads::user_data
