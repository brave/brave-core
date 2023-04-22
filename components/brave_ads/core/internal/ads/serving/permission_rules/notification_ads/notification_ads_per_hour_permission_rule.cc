/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"

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

constexpr base::TimeDelta kTimeConstraint = base::Hours(1);

bool DoesRespectCap(const std::vector<base::Time>& history) {
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();
  if (ads_per_hour == 0) {
    // Never respect cap if set to 0
    return false;
  }

  return DoesHistoryRespectRollingTimeConstraint(history, kTimeConstraint,
                                                 /*cap*/ ads_per_hour);
}

}  // namespace

base::expected<void, std::string> AdsPerHourPermissionRule::ShouldAllow()
    const {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never exceed the
    // maximum ads per hour
    return base::ok();
  }

  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kServed);
  if (!DoesRespectCap(history)) {
    return base::unexpected(
        "You have exceeded the allowed notification ads per hour");
  }

  return base::ok();
}

}  // namespace brave_ads::notification_ads
