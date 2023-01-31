/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/text_embedding_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads::targeting::features {

namespace {

constexpr char kFeatureName[] = "TextEmbedding";

constexpr char kFieldTrialParameterResourceVersion[] =
    "text_embedding_resource_version";
constexpr int kDefaultResourceVersion = 1;

constexpr char kFieldTrialParameterTextEmbeddingsHistorySize[] =
    "text_embeddings_history_size";
constexpr int kDefaultTextEmbeddingsHistorySize = 10;

}  // namespace

BASE_FEATURE(kTextEmbedding, kFeatureName, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsTextEmbeddingEnabled() {
  return base::FeatureList::IsEnabled(kTextEmbedding);
}

int GetTextEmbeddingsHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kTextEmbedding, kFieldTrialParameterTextEmbeddingsHistorySize,
      kDefaultTextEmbeddingsHistorySize);
}

int GetTextEmbeddingResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kTextEmbedding,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace ads::targeting::features
