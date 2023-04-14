/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_features.h"

namespace brave_ads::targeting {

BASE_FEATURE(kTextEmbeddingFeature,
             "TextEmbedding",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsTextEmbeddingEnabled() {
  return base::FeatureList::IsEnabled(kTextEmbeddingFeature);
}

}  // namespace brave_ads::targeting
