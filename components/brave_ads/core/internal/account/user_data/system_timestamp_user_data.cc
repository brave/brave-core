/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/system_timestamp_user_data.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

namespace brave_ads {

namespace {
constexpr char kSystemTimestampKey[] = "systemTimestamp";
}  // namespace

base::Value::Dict BuildSystemTimestampUserData() {
  base::Value::Dict user_data;

  user_data.Set(kSystemTimestampKey,
                TimeToPrivacyPreservingISO8601(base::Time::Now()));

  return user_data;
}

}  // namespace brave_ads
