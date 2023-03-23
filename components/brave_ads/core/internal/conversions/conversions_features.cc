/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"

#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::features {

namespace {

constexpr char kResourceVersionFieldTrialParamName[] = "resource_version";
constexpr int kResourceVersionDefaultValue = 1;

constexpr char kConversionIdPatternFieldTrialParamName[] =
    "conversion_id_pattern";
constexpr char kConversionIdPatternDefaultValue[] =
    R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~";

}  // namespace

BASE_FEATURE(kConversions, "Conversions", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsConversionsEnabled() {
  return base::FeatureList::IsEnabled(kConversions);
}

int GetConversionsResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kConversions,
                                          kResourceVersionFieldTrialParamName,
                                          kResourceVersionDefaultValue);
}

std::string GetConversionIdPattern() {
  return GetFieldTrialParamByFeatureAsString(
      kConversions, kConversionIdPatternFieldTrialParamName,
      kConversionIdPatternDefaultValue);
}

}  // namespace brave_ads::features
