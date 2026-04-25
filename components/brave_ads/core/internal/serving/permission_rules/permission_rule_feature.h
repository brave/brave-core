/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kPermissionRulesFeature);

inline constexpr base::FeatureParam<bool> kShouldOnlyServeAdsInWindowedMode{
    &kPermissionRulesFeature, "should_only_serve_ads_in_windowed_mode", true};

inline constexpr base::FeatureParam<bool>
    kShouldOnlyServeAdsWithValidInternetConnection{
        &kPermissionRulesFeature,
        "should_only_serve_ads_with_valid_internet_connection", true};

inline constexpr base::FeatureParam<bool>
    kShouldOnlyServeAdsIfMediaIsNotPlaying{
        &kPermissionRulesFeature,
        "should_only_serve_ads_if_media_is_not_playing", true};

inline constexpr base::FeatureParam<bool> kShouldOnlyServeAdsIfBrowserIsActive{
    &kPermissionRulesFeature, "should_only_serve_ads_if_browser_is_active",
    true};

inline constexpr base::FeatureParam<int> kDoNotDisturbFromHour{
    &kPermissionRulesFeature, "do_not_disturb_from_hour", 21};

inline constexpr base::FeatureParam<int> kDoNotDisturbToHour{
    &kPermissionRulesFeature, "do_not_disturb_to_hour", 6};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULE_FEATURE_H_
