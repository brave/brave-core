/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/targeting/models/contextual/text_classification/text_classification_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/targeting/processors/contextual/text_classification/text_classification_processor_constants.h"

namespace ads {
namespace features {

namespace {

constexpr char kFeatureName[] = "TextClassification";
constexpr char kFieldTrialParameterPageProbabilitiesHistorySize[] =
    "page_probabilities_history_size";
constexpr char kFieldTrialParameterResourceVersion[] =
    "text_classification_resource_version";
constexpr int kDefaultResourceVersion = 1;

}  // namespace

const base::Feature kTextClassification{kFeatureName,
                                        base::FEATURE_ENABLED_BY_DEFAULT};

bool IsTextClassificationEnabled() {
  return base::FeatureList::IsEnabled(kTextClassification);
}

int GetTextClassificationProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextClassification, kFieldTrialParameterPageProbabilitiesHistorySize,
      targeting::processor::kDefaultTextClassificationProbabilitiesHistorySize);
}

int GetTextClassificationResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextClassification,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace features
}  // namespace ads
