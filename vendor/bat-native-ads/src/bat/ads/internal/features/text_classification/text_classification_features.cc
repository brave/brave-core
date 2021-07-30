/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/text_classification/text_classification_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor_values.h"

namespace ads {
namespace features {

namespace {
const char kFeatureName[] = "TextClassification";
const char kFieldTrialParameterPageProbabilitiesHistorySize[] =
    "page_probabilities_history_size";
const char kFieldTrialParameterResourceVersion[] =
    "text_classification_resource_version";
const int kDefaultResourceVersion = 1;
}  // namespace

const base::Feature kTextClassification{kFeatureName,
                                        base::FEATURE_ENABLED_BY_DEFAULT};

bool IsTextClassificationEnabled() {
  return base::FeatureList::IsEnabled(kTextClassification);
}

int GetTextClassificationProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextClassification, kFieldTrialParameterPageProbabilitiesHistorySize,
      ad_targeting::processor::
          kDefaultTextClassificationProbabilitiesHistorySize);
}

int GetTextClassificationResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextClassification,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace features
}  // namespace ads
