/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/features/epsilon_greedy_bandit_features.h"
#include "brave/components/brave_ads/core/internal/features/purchase_intent_features.h"
#include "brave/components/brave_ads/core/internal/features/text_classification_features.h"
#include "brave/components/brave_ads/core/internal/features/text_embedding_features.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

namespace brave_ads::targeting {

namespace {

void OnGetTextEmbeddingHtmlEvents(
    UserModelInfo user_model,
    BuildUserModelCallback callback,
    bool success,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  if (!success) {
    BLOG(1, "Failed to get text embedding events");
    std::move(callback).Run(user_model);
    return;
  }

  user_model.embeddings_history = text_embedding_html_events;

  std::move(callback).Run(user_model);
}

}  // namespace

void BuildUserModel(BuildUserModelCallback callback) {
  UserModelInfo user_model;

  if (features::IsTextClassificationEnabled()) {
    const model::TextClassification text_classification_model;
    user_model.interest_segments = text_classification_model.GetSegments();
  }

  if (features::IsEpsilonGreedyBanditEnabled()) {
    const model::EpsilonGreedyBandit epsilon_greedy_bandit_model;
    user_model.latent_interest_segments =
        epsilon_greedy_bandit_model.GetSegments();
  }

  if (features::IsPurchaseIntentEnabled()) {
    const model::PurchaseIntent purchase_intent_model;
    user_model.purchase_intent_segments = purchase_intent_model.GetSegments();
  }

  if (features::IsTextEmbeddingEnabled()) {
    GetTextEmbeddingHtmlEventsFromDatabase(base::BindOnce(
        &OnGetTextEmbeddingHtmlEvents, user_model, std::move(callback)));
  } else {
    std::move(callback).Run(user_model);
  }
}

}  // namespace brave_ads::targeting
