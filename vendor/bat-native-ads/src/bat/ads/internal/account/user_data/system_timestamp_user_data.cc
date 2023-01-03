/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/system_timestamp_user_data.h"

#include "base/time/time.h"
#include "bat/ads/internal/common/time/time_util.h"

namespace ads::user_data {

namespace {
constexpr char kSystemTimestampKey[] = "systemTimestamp";
}  // namespace

base::Value::Dict GetSystemTimestamp() {
  base::Value::Dict user_data;

  user_data.Set(kSystemTimestampKey,
                TimeToPrivacyPreservingISO8601(base::Time::Now()));

  return user_data;
}

}  // namespace ads::user_data
