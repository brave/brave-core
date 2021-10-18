/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include "bat/ads/internal/ad_targeting/ad_targeting_constants.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

namespace ads {
namespace ad_targeting {

SegmentList GetTopParentChildSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model, /* parent_only */ false);
}

SegmentList GetTopParentSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model, /* parent_only */ true);
}

SegmentList GetTopParentChildInterestSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.interest_segments, kTopInterestSegmentsCount,
                        /* parent_only */ false);
}

SegmentList GetTopParentInterestSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.interest_segments, kTopInterestSegmentsCount,
                        /* parent_only */ true);
}

SegmentList GetTopParentChildLatentInterestSegments(
    const UserModelInfo& user_model) {
  return GetTopSegments(user_model.latent_interest_segments,
                        kTopLatentInterestSegmentsCount,
                        /* parent_only */ false);
}

SegmentList GetTopParentLatentInterestSegments(
    const UserModelInfo& user_model) {
  return GetTopSegments(user_model.latent_interest_segments,
                        kTopLatentInterestSegmentsCount,
                        /* parent_only */ true);
}

SegmentList GetTopParentChildPurchaseIntentSegments(
    const UserModelInfo& user_model) {
  return GetTopSegments(user_model.purchase_intent_segments,
                        kTopPurchaseIntentSegmentsCount,
                        /* parent_only */ false);
}

SegmentList GetTopParentPurchaseIntenSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.purchase_intent_segments,
                        kTopPurchaseIntentSegmentsCount,
                        /* parent_only */ true);
}

}  // namespace ad_targeting
}  // namespace ads
