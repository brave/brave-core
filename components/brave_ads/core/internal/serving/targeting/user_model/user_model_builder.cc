/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"

namespace brave_ads {

namespace {

void BuildTextEmbeddingHtmlEventsCallback(
    UserModelInfo user_model,
    BuildUserModelCallback callback,
    const bool success,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  if (!success) {
    BLOG(1, "Failed to get text embedding HTML events");
    return std::move(callback).Run(user_model);
  }

  user_model.interest.text_embedding_html_events = text_embedding_html_events;

  std::move(callback).Run(user_model);
}

void BuildTextEmbeddingHtmlEvents(UserModelInfo user_model,
                                  BuildUserModelCallback callback) {
  if (!base::FeatureList::IsEnabled(kTextEmbeddingFeature)) {
    return std::move(callback).Run(user_model);
  }

  GetTextEmbeddingHtmlEventsFromDatabase(base::BindOnce(
      &BuildTextEmbeddingHtmlEventsCallback, user_model, std::move(callback)));
}

}  // namespace

void BuildUserModel(BuildUserModelCallback callback) {
  UserModelInfo user_model;
  user_model.intent.segments = BuildIntentSegments();
  user_model.latent_interest.segments = BuildLatentInterestSegments();
  user_model.interest.segments = BuildInterestSegments();

  // TODO(https://github.com/brave/brave-browser/issues/33200): Decouple to
  // `BuildInterestSegments()`.
  BuildTextEmbeddingHtmlEvents(user_model, std::move(callback));
}

}  // namespace brave_ads
