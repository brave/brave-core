/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_day_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/base/time/time_constraint_util.h"

namespace ads::notification_ads {

namespace {

constexpr base::TimeDelta kTimeConstraint = base::Days(1);

bool DoesRespectCap(const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, kTimeConstraint, features::GetMaximumNotificationAdsPerDay());
}

}  // namespace

bool AdsPerDayPermissionRule::ShouldAllow() {
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed notification ads per day";
    return false;
  }

  return true;
}

const std::string& AdsPerDayPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads::notification_ads
