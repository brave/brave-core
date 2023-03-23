/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_FEATURES_H_

#include "base/feature_list.h"

namespace brave_ads::search_result_ads::features {

BASE_DECLARE_FEATURE(kFeature);

bool IsEnabled();
int GetMaximumAdsPerHour();
int GetMaximumAdsPerDay();

}  // namespace brave_ads::search_result_ads::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_FEATURES_H_
