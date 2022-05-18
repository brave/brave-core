/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/base/field_trial_params_util.h"

namespace ads {
namespace features {

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

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsConversionsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

int GetConversionsResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(
      kFeature, kFieldTrialParameterResourceVersion, kDefaultResourceVersion);
}

std::string GetDefaultConversionIdPattern() {
  return GetFieldTrialParamByFeatureAsString(
      kFeature, kFieldTrialParameterDefaultConversionIdPattern,
      kDefaultConversionIdPattern);
}

}  // namespace features
}  // namespace ads
