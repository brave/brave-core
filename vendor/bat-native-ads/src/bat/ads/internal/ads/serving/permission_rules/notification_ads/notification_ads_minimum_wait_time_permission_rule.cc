/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/base/platform/platform_helper.h"
#include "bat/ads/internal/base/time/time_constraint_util.h"
#include "bat/ads/internal/settings/settings.h"

namespace ads {
namespace notification_ads {

namespace {
constexpr int kMinimumWaitTimeCap = 1;
}  // namespace

MinimumWaitTimePermissionRule::MinimumWaitTimePermissionRule() = default;

MinimumWaitTimePermissionRule::~MinimumWaitTimePermissionRule() = default;

bool MinimumWaitTimePermissionRule::ShouldAllow() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    // Ads are periodically served on mobile so they will never be served before
    // the minimum wait time has passed
    return true;
  }

  const std::vector<base::Time>& history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ =
        "Notification ad cannot be shown as minimum wait time has not passed";
    return false;
  }

  return true;
}

std::string MinimumWaitTimePermissionRule::GetLastMessage() const {
  return last_message_;
}

bool MinimumWaitTimePermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  const int ads_per_hour = settings::GetNotificationAdsPerHour();
  if (ads_per_hour == 0) {
    return false;
  }

  const base::TimeDelta time_constraint =
      base::Seconds(base::Time::kSecondsPerHour / ads_per_hour);

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint,
                                                 kMinimumWaitTimeCap);
}

}  // namespace notification_ads
}  // namespace ads
