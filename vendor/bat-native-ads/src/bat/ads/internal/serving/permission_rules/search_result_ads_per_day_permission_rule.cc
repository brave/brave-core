/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/search_result_ads_per_day_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/time_constraint_util.h"
#include "bat/ads/internal/serving/serving_features.h"

namespace ads {

SearchResultAdsPerDayPermissionRule::SearchResultAdsPerDayPermissionRule() =
    default;

SearchResultAdsPerDayPermissionRule::~SearchResultAdsPerDayPermissionRule() =
    default;

bool SearchResultAdsPerDayPermissionRule::ShouldAllow() {
  const std::vector<base::Time>& history =
      GetAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed search result ads per day";
    return false;
  }

  return true;
}

std::string SearchResultAdsPerDayPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool SearchResultAdsPerDayPermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  const base::TimeDelta time_constraint = base::Days(1);

  const int cap = features::GetMaximumSearchResultAdsPerDay();

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint, cap);
}

}  // namespace ads
