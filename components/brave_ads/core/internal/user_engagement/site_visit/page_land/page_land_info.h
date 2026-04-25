/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_PAGE_LAND_PAGE_LAND_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_PAGE_LAND_PAGE_LAND_INFO_H_

#include <optional>

#include "base/timer/timer.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

struct PageLandInfo {
  AdInfo ad;
  base::RetainingOneShotTimer timer;
  std::optional<base::TimeDelta> remaining_time;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_PAGE_LAND_PAGE_LAND_INFO_H_
