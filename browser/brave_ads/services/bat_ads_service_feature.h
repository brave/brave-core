/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FEATURE_H_
#define BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FEATURE_H_

#include "base/feature.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kInProcessBraveAdsService);

inline constexpr base::FeatureParam<base::TimeDelta>
    kBraveAdsServiceStartupDelay{&kInProcessBraveAdsService, "startup_delay",
                                 base::Seconds(0)};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_FEATURE_H_
