/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ad_type.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

namespace brave_ads {

namespace {

constexpr int kMinimumWaitTimeCap = 1;

bool DoesRespectCap(const std::vector<base::Time>& history) {
  const int ads_per_hour = GetMaximumNotificationAdsPerHour();
  if (ads_per_hour == 0) {
    return false;
  }

  return DoesHistoryRespectRollingTimeConstraint(
      history, /*time_constraint*/ base::Hours(1) / ads_per_hour,
      kMinimumWaitTimeCap);
}

}  // namespace

base::expected<void, std::string>
NotificationAdMinimumWaitTimePermissionRule::ShouldAllow() const {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never be served before
    // the minimum wait time has passed
    return base::ok();
  }

  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kServed);
  if (!DoesRespectCap(history)) {
    return base::unexpected(
        "Notification ad cannot be shown as minimum wait time has not passed");
  }

  return base::ok();
}

}  // namespace brave_ads
