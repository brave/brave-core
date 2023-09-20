/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_builder.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/model/epsilon_greedy_bandit_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_model.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"

namespace brave_ads {

namespace {

void GetTextEmbeddingHtmlEventsCallback(
    UserModelInfo user_model,
    BuildUserModelCallback callback,
    const bool success,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  if (!success) {
    BLOG(1, "Failed to get text embedding HTML events");
    return std::move(callback).Run(user_model);
  }

  user_model.text_embedding_html_events = text_embedding_html_events;

  std::move(callback).Run(user_model);
}

}  // namespace

void BuildUserModel(BuildUserModelCallback callback) {
  UserModelInfo user_model;

  if (IsPurchaseIntentFeatureEnabled()) {
    user_model.intent_segments = GetPurchaseIntentSegments();
  }

  if (IsEpsilonGreedyBanditFeatureEnabled()) {
    user_model.latent_interest_segments = GetEpsilonGreedyBanditSegments();
  }

  if (IsTextClassificationFeatureEnabled()) {
    user_model.interest_segments = GetTextClassificationSegments();
  }

  if (!IsTextEmbeddingFeatureEnabled()) {
    return std::move(callback).Run(user_model);
  }

  GetTextEmbeddingHtmlEventsFromDatabase(base::BindOnce(
      &GetTextEmbeddingHtmlEventsCallback, user_model, std::move(callback)));
}

}  // namespace brave_ads
