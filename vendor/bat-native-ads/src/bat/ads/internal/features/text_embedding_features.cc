/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/text_embedding_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads {
namespace targeting {
namespace features {

namespace {

constexpr char kFeatureName[] = "TextEmbedding";

constexpr char kFieldTrialParameterPageEmbeddingsHistorySize[] =
    "page_embeddings_history_size";
const int kDefaultPageEmbeddingsHistorySize = 10;

constexpr char kFieldTrialParameterResourceVersion[] =
    "text_embedding_resource_version";

constexpr int kDefaultResourceVersion = 1;

}  // namespace

const base::Feature kTextEmbedding{kFeatureName,
                                        base::FEATURE_ENABLED_BY_DEFAULT};

bool IsTextEmbeddingEnabled() {
  return base::FeatureList::IsEnabled(kTextEmbedding);
}

int GetTextEmbeddingsHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextEmbedding, kFieldTrialParameterPageEmbeddingsHistorySize,
      kDefaultPageEmbeddingsHistorySize);
}

int GetTextEmbeddingResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextEmbedding,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace features
}  // namespace targeting
}  // namespace ads
