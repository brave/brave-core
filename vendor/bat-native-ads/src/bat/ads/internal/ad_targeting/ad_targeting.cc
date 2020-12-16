/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdTargeting::AdTargeting() = default;

AdTargeting::~AdTargeting() = default;

SegmentList AdTargeting::GetSegments() const {
  // TODO(Moritz Haller): Having variant-conditional path here means all
  // classifiers are still initialized but only categories of the enabled
  // feature gets slotted into winning categories >> SOLID refactor solves that

  // TODO(Moritz Haller): how to enable/disable features via cli args?

  // Enabled by default
  SegmentList text_classification_segments;
  if (features::IsTextClassificationModelEnabled()) {
    text_classification_segments = GetTextClassificationSegments();
  }

  // Enabled by default
  SegmentList purchase_intent_segments;
  if (features::IsPurchaseIntentModelEnabled()) {
    purchase_intent_segments = GetPurchaseIntentSegments();
  }

  // Disabled by default
  SegmentList epsilon_greedy_segments;
  if (features::IsEpsilonGreedyBanditEnabled()) {
    epsilon_greedy_segments = GetEpsilonGreedyBanditSegments();
  }

  SegmentList segments = text_classification_segments;
  segments.insert(segments.end(), purchase_intent_segments.begin(),
      purchase_intent_segments.end());
  segments.insert(segments.end(), epsilon_greedy_segments.begin(),
      epsilon_greedy_segments.end());

  return segments;
}

///////////////////////////////////////////////////////////////////////////////

SegmentList AdTargeting::GetTextClassificationSegments() const {
  ad_targeting::model::TextClassification model;
  return model.GetSegments();
}

SegmentList AdTargeting::GetPurchaseIntentSegments() const {
  ad_targeting::model::PurchaseIntent model;
  return model.GetSegments();
}

SegmentList AdTargeting::GetEpsilonGreedyBanditSegments() const {
  ad_targeting::model::EpsilonGreedyBandit model;
  return model.GetSegments();
}


}  // namespace ads
