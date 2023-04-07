/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::new_tab_page_ads::features {

namespace {

constexpr char kVersionFieldTrialParamName[] = "version";
constexpr int kVersionDefaultValue = 2;

}  // namespace

BASE_FEATURE(kServing, "NewTabPageAdServing", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsServingEnabled() {
  return base::FeatureList::IsEnabled(kServing);
}

int GetServingVersion() {
  return GetFieldTrialParamByFeatureAsInt(kServing, kVersionFieldTrialParamName,
                                          kVersionDefaultValue);
}

}  // namespace brave_ads::new_tab_page_ads::features
