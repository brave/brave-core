/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/permission_rule_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/base/field_trial_params_util.h"

namespace ads {
namespace permission_rules {
namespace features {

namespace {

constexpr char kFeatureName[] = "PermissionRules";

constexpr char kFieldTrialParameterShouldOnlyServeAdsInWindowedMode[] =
    "should_only_serve_ads_in_windowed_mode";
constexpr bool kDefaultShouldOnlyServeAdsInWindowedMode = true;

constexpr char
    kFieldTrialParameterShouldOnlyServeAdsWithValidInternetConnection[] =
        "should_only_serve_ads_with_valid_internet_connection";
constexpr bool kDefaultShouldOnlyServeAdsWithValidInternetConnection = true;

constexpr char kFieldTrialParameterShouldOnlyServeAdsIfMediaIsNotPlaying[] =
    "should_only_serve_ads_if_media_is_not_playing";
constexpr bool kDefaultShouldOnlyServeAdsIfMediaIsNotPlaying = true;

constexpr char kFieldTrialParameterShouldOnlyServeAdsIfBrowserIsActive[] =
    "should_only_serve_ads_if_browser_is_active";
constexpr bool kDefaultShouldOnlyServeAdsIfBrowserIsActive = true;

}  // namespace

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
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

}  // namespace features
}  // namespace permission_rules
}  // namespace ads
