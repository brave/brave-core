/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_CAMPAIGN_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_CAMPAIGN_INFO_H_

#include <string>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-data-view.h"

namespace brave_ads {

struct CreativeCampaignInfo final {
  mojom::NewTabPageAdMetricType metric_type =
      mojom::NewTabPageAdMetricType::kUndefined;
  base::Time start_at;
  base::Time end_at;
  int daily_cap = 0;
  std::string advertiser_id;
  int priority = 0;
  double pass_through_rate = 0.0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_CAMPAIGN_INFO_H_
