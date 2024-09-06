/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

bool HasNotificationAdsPerHourPermission() {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never exceed the
    // maximum ads per hour.
    return true;
  }

  if (!DoesHistoryRespectRollingTimeConstraint(
          mojom::AdType::kNotificationAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/GetMaximumNotificationAdsPerHour())) {
    BLOG(2, "You have exceeded the allowed notification ads per hour");
    return false;
  }

  return true;
}

}  // namespace brave_ads
