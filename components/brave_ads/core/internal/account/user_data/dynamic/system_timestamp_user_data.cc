/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/system_timestamp_user_data.h"

#include <string_view>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr std::string_view kSystemTimestampKey = "systemTimestamp";
}  // namespace

base::Value::Dict BuildSystemTimestampUserData() {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  return base::Value::Dict().Set(
      kSystemTimestampKey, TimeToPrivacyPreservingIso8601(base::Time::Now()));
}

}  // namespace brave_ads
