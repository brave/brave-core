/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/text_classification_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads::targeting::features {

namespace {

constexpr char kFeatureName[] = "TextClassification";

constexpr char kFieldTrialParameterPageProbabilitiesHistorySize[] =
    "page_probabilities_history_size";
constexpr int kDefaultPageProbabilitiesHistorySize = 5;

constexpr char kFieldTrialParameterResourceVersion[] =
    "text_classification_resource_version";

constexpr int kDefaultResourceVersion = 1;

}  // namespace

BASE_FEATURE(kTextClassification,
             kFeatureName,
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsTextClassificationEnabled() {
  return base::FeatureList::IsEnabled(kTextClassification);
}

int GetTextClassificationProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextClassification, kFieldTrialParameterPageProbabilitiesHistorySize,
      kDefaultPageProbabilitiesHistorySize);
}

int GetTextClassificationResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextClassification,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace ads::targeting::features
