/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/features/text_classification_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::targeting::features {

namespace {

constexpr char kResourceVersionFieldTrialParamName[] =
    "text_classification_resource_version";
constexpr int kResourceVersionDefaultValue = 1;

constexpr char kPageProbabilitiesHistorySizeFieldTrialParamName[] =
    "page_probabilities_history_size";
constexpr int kPageProbabilitiesHistorySizeDefaultValue = 5;

}  // namespace

BASE_FEATURE(kTextClassification,
             "TextClassification",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsTextClassificationEnabled() {
  return base::FeatureList::IsEnabled(kTextClassification);
}

int GetTextClassificationResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextClassification,
                                          kResourceVersionFieldTrialParamName,
                                          kResourceVersionDefaultValue);
}

int GetTextClassificationProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextClassification, kPageProbabilitiesHistorySizeFieldTrialParamName,
      kPageProbabilitiesHistorySizeDefaultValue);
}

}  // namespace brave_ads::targeting::features
