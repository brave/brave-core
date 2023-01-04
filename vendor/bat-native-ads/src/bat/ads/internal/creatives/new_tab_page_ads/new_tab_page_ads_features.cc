/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ads_features.h"

namespace ads::new_tab_page_ads::features {

namespace {
constexpr char kFeatureName[] = "NewTabPageAds";
}  // namespace

BASE_FEATURE(kFeature, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

}  // namespace ads::new_tab_page_ads::features
