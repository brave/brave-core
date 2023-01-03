/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_minimum_wait_time_permission_rule.h"

#include <vector>

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/common/time/time_constraint_util.h"

namespace ads::new_tab_page_ads {

namespace {

constexpr int kMinimumWaitTimeCap = 1;

bool DoesRespectCap(const std::vector<base::Time>& history) {
  return DoesHistoryRespectRollingTimeConstraint(
      history, features::GetNewTabPageAdsMinimumWaitTime(),
      kMinimumWaitTimeCap);
}

}  // namespace

bool MinimumWaitTimePermissionRule::ShouldAllow() {
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNewTabPageAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ =
        "New tab page ad cannot be shown as minimum wait time has not passed";
    return false;
  }

  return true;
}

const std::string& MinimumWaitTimePermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads::new_tab_page_ads
