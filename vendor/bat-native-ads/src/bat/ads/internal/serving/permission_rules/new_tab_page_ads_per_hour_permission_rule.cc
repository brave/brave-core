/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/new_tab_page_ads_per_hour_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/time_constraint_util.h"
#include "bat/ads/internal/serving/serving_features.h"

namespace ads {

NewTabPageAdsPerHourPermissionRule::NewTabPageAdsPerHourPermissionRule() =
    default;

NewTabPageAdsPerHourPermissionRule::~NewTabPageAdsPerHourPermissionRule() =
    default;

bool NewTabPageAdsPerHourPermissionRule::ShouldAllow() {
  const std::vector<base::Time>& history =
      GetAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed new tab page ads per hour";
    return false;
  }

  return true;
}

std::string NewTabPageAdsPerHourPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool NewTabPageAdsPerHourPermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  const base::TimeDelta time_constraint = base::Hours(1);

  const int cap = features::GetMaximumNewTabPageAdsPerHour();

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint, cap);
}

}  // namespace ads
