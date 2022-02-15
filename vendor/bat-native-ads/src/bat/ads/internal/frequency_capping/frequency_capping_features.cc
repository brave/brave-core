/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/features/features_util.h"

namespace ads {
namespace features {
namespace frequency_capping {

namespace {

const char kFeatureName[] = "FrequencyCapping";

const char kFieldTrialParameterShouldExcludeAdIfConverted[] =
    "should_exclude_ad_if_converted";
const bool kDefaultShouldExcludeAdIfConverted = true;

const char kFieldTrialParameterExcludeAdIfDismissedWithinTimeWindow[] =
    "exclude_ad_if_dismissed_within_time_window";
const base::TimeDelta kDefaultExcludeAdIfDismissedWithinTimeWindow =
    base::Days(2);

const char kFieldTrialParameterExcludeAdIfTransferredWithinTimeWindow[] =
    "exclude_ad_if_transferred_within_time_window";
const base::TimeDelta kDefaultExcludeAdIfTransferredWithinTimeWindow =
    base::Days(2);

const char kFieldTrialParameterShouldOnlyServeAdsInWindowedMode[] =
    "should_only_serve_ads_in_windowed_mode";
const bool kDefaultShouldOnlyServeAdsInWindowedMode = true;

const char kFieldTrialParameterShouldOnlyServeAdsWithValidInternetConnection[] =
    "should_only_serve_ads_with_valid_internet_connection";
const bool kDefaultShouldOnlyServeAdsWithValidInternetConnection = true;

const char kFieldTrialParameterShouldOnlyServeAdsIfMediaIsNotPlaying[] =
    "should_only_serve_ads_if_media_is_not_playing";
const bool kDefaultShouldOnlyServeAdsIfMediaIsNotPlaying = true;

const char kFieldTrialParameterShouldOnlyServeAdsIfBrowserIsActive[] =
    "should_only_serve_ads_if_browser_is_active";
const bool kDefaultShouldOnlyServeAdsIfBrowserIsActive = true;

}  // namespace

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

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

bool ShouldOnlyServeAdsInWindowedMode() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldOnlyServeAdsInWindowedMode,
      kDefaultShouldOnlyServeAdsInWindowedMode);
}

bool ShouldOnlyServeAdsWithValidInternetConnection() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature,
      kFieldTrialParameterShouldOnlyServeAdsWithValidInternetConnection,
      kDefaultShouldOnlyServeAdsWithValidInternetConnection);
}

bool ShouldOnlyServeAdsIfMediaIsNotPlaying() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldOnlyServeAdsIfMediaIsNotPlaying,
      kDefaultShouldOnlyServeAdsIfMediaIsNotPlaying);
}

bool ShouldOnlyServeAdsIfBrowserIsActive() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldOnlyServeAdsIfBrowserIsActive,
      kDefaultShouldOnlyServeAdsIfBrowserIsActive);
}

}  // namespace frequency_capping
}  // namespace features
}  // namespace ads
