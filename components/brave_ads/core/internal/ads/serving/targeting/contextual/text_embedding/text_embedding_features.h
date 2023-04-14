/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads::targeting {

BASE_DECLARE_FEATURE(kTextEmbeddingFeature);

bool IsTextEmbeddingEnabled();

constexpr base::FeatureParam<int> kTextEmbeddingResourceVersion{
    &kTextEmbeddingFeature, "resource_version", 1};

constexpr base::FeatureParam<int> kTextEmbeddingHistorySize{
    &kTextEmbeddingFeature, "history_size", 10};

}  // namespace brave_ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_FEATURES_H_
