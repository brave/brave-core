/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/conversions/conversions_features.h"

#include "bat/ads/internal/features/features_util.h"

namespace ads {
namespace features {

namespace {

const char kFeatureName[] = "Conversions";

const char kFieldTrialParameterResourceVersion[] =
    "conversions_resource_version";
const int kDefaultResourceVersion = 1;

const char kFieldTrialParameterDefaultConversionIdPattern[] =
    "conversions_default_conversion_id_pattern";
const char kDefaultDefaultConversionIdPattern[] =
    "<meta.*name=\"ad-conversion-id\".*content=\"(.*)\".*>";

}  // namespace

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsConversionsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

int GetConversionsResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(
      kFeature, kFieldTrialParameterResourceVersion, kDefaultResourceVersion);
}

std::string GetGetDefaultConversionIdPattern() {
  return GetFieldTrialParamByFeatureAsString(
      kFeature, kFieldTrialParameterDefaultConversionIdPattern,
      kDefaultDefaultConversionIdPattern);
}

}  // namespace features
}  // namespace ads
