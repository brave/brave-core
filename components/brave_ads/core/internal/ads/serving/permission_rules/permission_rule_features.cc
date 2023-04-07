/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::permission_rules::features {

namespace {

constexpr char kShouldOnlyServeAdsInWindowedModeFieldTrialParamName[] =
    "should_only_serve_ads_in_windowed_mode";
constexpr bool kShouldOnlyServeAdsInWindowedModeDefaultValue = true;

constexpr char
    kShouldOnlyServeAdsWithValidInternetConnectionFieldTrialParamName[] =
        "should_only_serve_ads_with_valid_internet_connection";
constexpr bool kShouldOnlyServeAdsWithValidInternetConnectionDefaultValue =
    true;

constexpr char kShouldOnlyServeAdsIfMediaIsNotPlayingFieldTrialParamName[] =
    "should_only_serve_ads_if_media_is_not_playing";
constexpr bool kShouldOnlyServeAdsIfMediaIsNotPlayingDefaultValue = true;

constexpr char kShouldOnlyServeAdsIfBrowserIsActiveFieldTrialParamName[] =
    "should_only_serve_ads_if_browser_is_active";
constexpr bool kShouldOnlyServeAdsIfBrowserIsActiveDefaultValue = true;

}  // namespace

BASE_FEATURE(kFeature, "PermissionRules", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

bool ShouldOnlyServeAdsInWindowedMode() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kShouldOnlyServeAdsInWindowedModeFieldTrialParamName,
      kShouldOnlyServeAdsInWindowedModeDefaultValue);
}

bool ShouldOnlyServeAdsWithValidInternetConnection() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature,
      kShouldOnlyServeAdsWithValidInternetConnectionFieldTrialParamName,
      kShouldOnlyServeAdsWithValidInternetConnectionDefaultValue);
}

bool ShouldOnlyServeAdsIfMediaIsNotPlaying() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kShouldOnlyServeAdsIfMediaIsNotPlayingFieldTrialParamName,
      kShouldOnlyServeAdsIfMediaIsNotPlayingDefaultValue);
}

bool ShouldOnlyServeAdsIfBrowserIsActive() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kShouldOnlyServeAdsIfBrowserIsActiveFieldTrialParamName,
      kShouldOnlyServeAdsIfBrowserIsActiveDefaultValue);
}

}  // namespace brave_ads::permission_rules::features
