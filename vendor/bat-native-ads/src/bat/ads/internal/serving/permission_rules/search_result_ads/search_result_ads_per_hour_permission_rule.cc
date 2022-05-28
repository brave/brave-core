/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/search_result_ads/search_result_ads_per_hour_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/time_constraint_util.h"
#include "bat/ads/internal/serving/serving_features.h"

namespace ads {

namespace {
constexpr base::TimeDelta kTimeConstraint = base::Hours(1);
}  // namespace

SearchResultAdsPerHourPermissionRule::SearchResultAdsPerHourPermissionRule() =
    default;

SearchResultAdsPerHourPermissionRule::~SearchResultAdsPerHourPermissionRule() =
    default;

bool SearchResultAdsPerHourPermissionRule::ShouldAllow() {
  const std::vector<base::Time>& history =
      GetAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed search result ads per hour";
    return false;
  }

  return true;
}

std::string SearchResultAdsPerHourPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool SearchResultAdsPerHourPermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, kTimeConstraint, features::GetMaximumSearchResultAdsPerHour());
}

}  // namespace ads
