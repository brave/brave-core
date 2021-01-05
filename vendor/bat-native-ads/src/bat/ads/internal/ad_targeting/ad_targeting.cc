/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"

namespace ads {

AdTargeting::AdTargeting() = default;

AdTargeting::~AdTargeting() = default;

SegmentList AdTargeting::GetSegments() const {
  SegmentList segments;

  if (features::IsTextClassificationEnabled()) {
    const ad_targeting::model::TextClassification text_classification_model;
    const SegmentList text_classification_segments =
        text_classification_model.GetSegments();
    segments.insert(segments.end(), text_classification_segments.begin(),
        text_classification_segments.end());
  }

  if (features::IsPurchaseIntentEnabled()) {
    const ad_targeting::model::PurchaseIntent purchase_intent_model;
    const SegmentList purchase_intent_segments =
        purchase_intent_model.GetSegments();
    segments.insert(segments.end(), purchase_intent_segments.begin(),
        purchase_intent_segments.end());
  }

  if (features::IsEpsilonGreedyBanditEnabled()) {
    const ad_targeting::model::EpsilonGreedyBandit epsilon_greedy_bandit_model;
    const SegmentList epsilon_greedy_bandit_segments =
        epsilon_greedy_bandit_model.GetSegments();
    segments.insert(segments.end(), epsilon_greedy_bandit_segments.begin(),
        epsilon_greedy_bandit_segments.end());
  }

  return segments;
}

}  // namespace ads
