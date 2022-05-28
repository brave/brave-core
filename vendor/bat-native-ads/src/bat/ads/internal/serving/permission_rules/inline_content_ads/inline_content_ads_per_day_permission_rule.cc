/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/inline_content_ads/inline_content_ads_per_day_permission_rule.h"

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/time_constraint_util.h"
#include "bat/ads/internal/serving/serving_features.h"

namespace ads {

namespace {
constexpr base::TimeDelta kTimeConstraint = base::Days(1);
}  // namespace

InlineContentAdsPerDayPermissionRule::InlineContentAdsPerDayPermissionRule() =
    default;

InlineContentAdsPerDayPermissionRule::~InlineContentAdsPerDayPermissionRule() =
    default;

bool InlineContentAdsPerDayPermissionRule::ShouldAllow() {
  const std::vector<base::Time>& history =
      GetAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed inline content ads per day";
    return false;
  }

  return true;
}

std::string InlineContentAdsPerDayPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool InlineContentAdsPerDayPermissionRule::DoesRespectCap(
    const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, kTimeConstraint, features::GetMaximumInlineContentAdsPerDay());
}

}  // namespace ads
