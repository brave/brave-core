/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/metrics/field_trial_params_util.h"

namespace ads::exclusion_rules::features {

namespace {

constexpr char kFeatureName[] = "FrequencyCapping";

constexpr char kFieldTrialParameterShouldExcludeAdIfConverted[] =
    "should_exclude_ad_if_converted";
constexpr bool kDefaultShouldExcludeAdIfConverted = true;

constexpr char kFieldTrialParameterExcludeAdIfDismissedWithinTimeWindow[] =
    "exclude_ad_if_dismissed_within_time_window";
constexpr base::TimeDelta kDefaultExcludeAdIfDismissedWithinTimeWindow =
    base::Hours(0);

constexpr char kFieldTrialParameterExcludeAdIfTransferredWithinTimeWindow[] =
    "exclude_ad_if_transferred_within_time_window";
constexpr base::TimeDelta kDefaultExcludeAdIfTransferredWithinTimeWindow =
    base::Hours(0);

}  // namespace

BASE_FEATURE(kFeature, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

bool ShouldExcludeAdIfConverted() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldExcludeAdIfConverted,
      kDefaultShouldExcludeAdIfConverted);
}

base::TimeDelta ExcludeAdIfDismissedWithinTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kFieldTrialParameterExcludeAdIfDismissedWithinTimeWindow,
      kDefaultExcludeAdIfDismissedWithinTimeWindow);
}

base::TimeDelta ExcludeAdIfTransferredWithinTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kFieldTrialParameterExcludeAdIfTransferredWithinTimeWindow,
      kDefaultExcludeAdIfTransferredWithinTimeWindow);
}

}  // namespace ads::exclusion_rules::features
