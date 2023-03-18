/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/features/features_util.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/features/epsilon_greedy_bandit_features.h"
#include "brave/components/brave_ads/core/internal/features/purchase_intent_features.h"
#include "brave/components/brave_ads/core/internal/features/text_classification_features.h"
#include "brave/components/brave_ads/core/internal/features/text_embedding_features.h"

namespace brave_ads {

void LogFeatures() {
  if (targeting::features::IsEpsilonGreedyBanditEnabled()) {
    BLOG(3, "Epsilon greedy bandit ad targeting feature is enabled");
  }

  if (targeting::features::IsPurchaseIntentEnabled()) {
    BLOG(3, "Purchase intent ad targeting feature is enabled");
  }

  if (targeting::features::IsTextClassificationEnabled()) {
    BLOG(3, "Text classification ad targeting feature is enabled");
  }

  if (targeting::features::IsTextEmbeddingEnabled()) {
    BLOG(3, "Text embedding ad targeting feature is enabled");
  }
}

}  // namespace brave_ads
