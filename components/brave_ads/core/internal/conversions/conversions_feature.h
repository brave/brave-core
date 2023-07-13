/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_FEATURE_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kConversionsFeature);

bool IsConversionFeatureEnabled();

constexpr base::FeatureParam<int> kConversionResourceVersion{
    &kConversionsFeature, "resource_version", 1};

constexpr base::FeatureParam<std::string> kHtmlMetaTagConversionIdPattern{
    &kConversionsFeature, "html_meta_tag_id_pattern",
    R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~"};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_FEATURE_H_
