/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_features.h"

namespace brave_ads::inline_content_ads {

BASE_FEATURE(kServingFeature,
             "InlineContentAdServing",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsServingEnabled() {
  return base::FeatureList::IsEnabled(kServingFeature);
}

}  // namespace brave_ads::inline_content_ads
