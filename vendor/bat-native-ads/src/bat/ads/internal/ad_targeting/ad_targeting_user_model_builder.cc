/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"

namespace ads {
namespace ad_targeting {

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

}  // namespace ad_targeting
}  // namespace ads
