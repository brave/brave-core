/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_model.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/features/epsilon_greedy_bandit_features.h"
#include "brave/components/brave_ads/core/internal/features/purchase_intent_features.h"
#include "brave/components/brave_ads/core/internal/features/text_classification_features.h"

namespace brave_ads::targeting {

UserModelInfo BuildUserModel() {
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

  return user_model;
}

}  // namespace brave_ads::targeting
