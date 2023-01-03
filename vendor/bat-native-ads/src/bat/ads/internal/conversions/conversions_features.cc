/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/common/metrics/field_trial_params_util.h"

namespace ads::features {

namespace {

constexpr char kFeatureName[] = "Conversions";

constexpr char kFieldTrialParameterResourceVersion[] =
    "conversions_resource_version";
constexpr int kDefaultResourceVersion = 1;

constexpr char kFieldTrialParameterDefaultConversionIdPattern[] =
    "conversions_default_conversion_id_pattern";
constexpr char kDefaultConversionIdPattern[] =
    "<meta.*name=\"ad-conversion-id\".*content=\"([-a-zA-Z0-9]*)\".*>";

}  // namespace

BASE_FEATURE(kConversions, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsConversionsEnabled() {
  return base::FeatureList::IsEnabled(kConversions);
}

int GetConversionsResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kConversions,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

std::string GetDefaultConversionIdPattern() {
  return GetFieldTrialParamByFeatureAsString(
      kConversions, kFieldTrialParameterDefaultConversionIdPattern,
      kDefaultConversionIdPattern);
}

}  // namespace ads::features
