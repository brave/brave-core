/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kTextClassificationFeature);

bool IsTextClassificationFeatureEnabled();

constexpr base::FeatureParam<int> kTextClassificationResourceVersion{
    &kTextClassificationFeature, "resource_version", 1};

constexpr base::FeatureParam<int>
    kTextClassificationPageProbabilitiesHistorySize{
        &kTextClassificationFeature, "page_probabilities_history_size", 5};

// V2 without exploration; 5 page history; Legacy text classifier.

// V2 with exploration; 25 page history; Legacy text classifier.
// V2 with exploration; 25 page history; New text classifier.
// V2 without exploration; 25 page history; Legacy text classifier.
// V2 without exploration; 25 page history; New text classifier.

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURE_H_
