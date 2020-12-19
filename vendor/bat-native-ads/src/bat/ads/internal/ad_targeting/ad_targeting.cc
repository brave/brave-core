/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/purchase_intent/purchase_intent_model.h"

namespace ads {

namespace {

SegmentList GetTextClassificationSegments() {
  ad_targeting::model::TextClassification model;
  return model.GetSegments();
}

SegmentList GetPurchaseIntentSegments() {
  ad_targeting::model::PurchaseIntent model;
  return model.GetSegments();
}

}  // namespace

AdTargeting::AdTargeting() = default;

AdTargeting::~AdTargeting() = default;

SegmentList AdTargeting::GetSegments() const {
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  const SegmentList purchase_intent_segments = GetPurchaseIntentSegments();

  SegmentList segments = text_classification_segments;
  segments.insert(segments.end(), purchase_intent_segments.begin(),
      purchase_intent_segments.end());

  return segments;
}

}  // namespace ads
