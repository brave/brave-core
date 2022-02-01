/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/new_tab_page_ads/new_tab_page_ads_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads {
namespace new_tab_page_ads {
namespace features {

namespace {
const char kFeatureName[] = "NewTabPageAds";
}  // namespace

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

}  // namespace features
}  // namespace new_tab_page_ads
}  // namespace ads
