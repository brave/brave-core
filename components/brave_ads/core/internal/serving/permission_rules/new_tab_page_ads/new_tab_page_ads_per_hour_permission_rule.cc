/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ads_per_hour_permission_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

bool HasNewTabPageAdsPerHourPermission() {
  if (!DoesHistoryRespectRollingTimeConstraint(
          mojom::AdType::kNewTabPageAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/kMaximumNewTabPageAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed new tab page ads per hour");
    return false;
  }

  return true;
}

}  // namespace brave_ads
