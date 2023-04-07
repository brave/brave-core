/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::exclusion_rules::features {

namespace {

constexpr char kShouldExcludeAdIfConvertedFieldTrialParamName[] =
    "should_exclude_ad_if_converted";
constexpr bool kShouldExcludeAdIfConvertedDefaultValue = true;

constexpr char kExcludeAdIfDismissedWithinTimeWindowFieldTrialParamName[] =
    "exclude_ad_if_dismissed_within_time_window";
constexpr base::TimeDelta kExcludeAdIfDismissedWithinTimeWindowDefaultValue =
    base::Hours(0);

constexpr char kExcludeAdIfTransferredWithinTimeWindowFieldTrialParamName[] =
    "exclude_ad_if_transferred_within_time_window";
constexpr base::TimeDelta kExcludeAdIfTransferredWithinTimeWindowDefaultValue =
    base::Hours(0);

}  // namespace

BASE_FEATURE(kFeature, "ExclusionRules", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

bool ShouldExcludeAdIfConverted() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kShouldExcludeAdIfConvertedFieldTrialParamName,
      kShouldExcludeAdIfConvertedDefaultValue);
}

base::TimeDelta GetExcludeAdIfDismissedWithinTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kExcludeAdIfDismissedWithinTimeWindowFieldTrialParamName,
      kExcludeAdIfDismissedWithinTimeWindowDefaultValue);
}

base::TimeDelta GetExcludeAdIfTransferredWithinTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kExcludeAdIfTransferredWithinTimeWindowFieldTrialParamName,
      kExcludeAdIfTransferredWithinTimeWindowDefaultValue);
}

}  // namespace brave_ads::exclusion_rules::features
