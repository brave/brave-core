/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_feature.h"

namespace brave_ads {

BASE_FEATURE(kNewTabPageAdFeature,
             "NewTabPageAds",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsNewTabPageAdFeatureEnabled() {
  return base::FeatureList::IsEnabled(kNewTabPageAdFeature);
}

}  // namespace brave_ads
