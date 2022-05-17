/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/ads_per_hour_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/base/time_constraint_util.h"
#include "bat/ads/internal/settings/settings.h"

namespace ads {

AdsPerHourPermissionRule::AdsPerHourPermissionRule() = default;

AdsPerHourPermissionRule::~AdsPerHourPermissionRule() = default;

bool AdsPerHourPermissionRule::ShouldAllow() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    // Ads are periodically served on mobile so they will never exceed the
    // maximum ads per hour
    return true;
  }

  const std::vector<base::Time> history =
      GetAdEvents(AdType::kAdNotification, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed ads per hour";
    return false;
  }

  return true;
}

std::string AdsPerHourPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool AdsPerHourPermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  const base::TimeDelta time_constraint = base::Hours(1);

  const int cap = settings::GetAdsPerHour();
  if (cap == 0) {
    return false;
  }

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint, cap);
}

}  // namespace ads
