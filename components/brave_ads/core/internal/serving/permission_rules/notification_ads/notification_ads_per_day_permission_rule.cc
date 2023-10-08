/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kTimeConstraint = base::Days(1);

bool DoesRespectCap(const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, kTimeConstraint,
      /*cap=*/kMaximumNotificationAdsPerDay.Get());
}

}  // namespace

base::expected<void, std::string>
NotificationAdsPerDayPermissionRule::ShouldAllow() const {
  const std::vector<base::Time> history =
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kServed);
  if (!DoesRespectCap(history)) {
    return base::unexpected(
        "You have exceeded the allowed notification ads per day");
  }

  return base::ok();
}

}  // namespace brave_ads
