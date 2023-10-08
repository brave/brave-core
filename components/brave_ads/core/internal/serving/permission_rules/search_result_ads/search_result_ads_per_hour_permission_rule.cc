/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/search_result_ads/search_result_ads_per_hour_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/search_result_ad/search_result_ad_feature.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kTimeConstraint = base::Hours(1);

bool DoesRespectCap(const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, kTimeConstraint,
      /*cap=*/kMaximumSearchResultAdsPerHour.Get());
}

}  // namespace

base::expected<void, std::string>
SearchResultAdsPerHourPermissionRule::ShouldAllow() const {
  const std::vector<base::Time> history =
      GetCachedAdEvents(AdType::kSearchResultAd, ConfirmationType::kServed);
  if (!DoesRespectCap(history)) {
    return base::unexpected(
        "You have exceeded the allowed search result ads per hour");
  }

  return base::ok();
}

}  // namespace brave_ads
