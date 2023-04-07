/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads::notification_ads {

namespace {

constexpr int kMinimumWaitTimeCap = 1;

bool DoesRespectCap(const std::vector<base::Time>& history) {
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();
  if (ads_per_hour == 0) {
    return false;
  }

  const base::TimeDelta time_constraint =
      base::Seconds(base::Time::kSecondsPerHour / ads_per_hour);

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint,
                                                 kMinimumWaitTimeCap);
}

}  // namespace

bool MinimumWaitTimePermissionRule::ShouldAllow() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    // Ads are periodically served on mobile so they will never be served before
    // the minimum wait time has passed
    return true;
  }

  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ =
        "Notification ad cannot be shown as minimum wait time has not passed";
    return false;
  }

  return true;
}

const std::string& MinimumWaitTimePermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads::notification_ads
