/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/inline_content_ads/inline_content_ads_per_hour_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_features.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

namespace brave_ads::inline_content_ads {

namespace {

constexpr base::TimeDelta kTimeConstraint = base::Hours(1);

bool DoesRespectCap(const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(history, kTimeConstraint,
                                                 kMaximumAdsPerHour.Get());
}

}  // namespace

bool AdsPerHourPermissionRule::ShouldAllow() {
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kInlineContentAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed inline content ads per hour";
    return false;
  }

  return true;
}

const std::string& AdsPerHourPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads::inline_content_ads
