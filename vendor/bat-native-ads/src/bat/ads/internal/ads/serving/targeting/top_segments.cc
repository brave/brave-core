/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/top_segments.h"

#include "bat/ads/internal/ads/serving/targeting/top_segments_util.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"

namespace ads::targeting {

namespace {

constexpr int kTopSegmentsMaxCount = 3;
constexpr int kTopInterestSegmentsMaxCount = 3;
constexpr int kTopLatentInterestSegmentsMaxCount = 3;
constexpr int kTopPurchaseIntentSegmentsMaxCount = 3;

}  // namespace

SegmentList GetTopChildSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model, kTopSegmentsMaxCount,
                        /*parent_only*/ false);
}

SegmentList GetTopParentSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model, kTopSegmentsMaxCount,
                        /*parent_only*/ true);
}

SegmentList GetTopChildInterestSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.interest_segments,
                        kTopInterestSegmentsMaxCount,
                        /*parent_only*/ false);
}

SegmentList GetTopParentInterestSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.interest_segments,
                        kTopInterestSegmentsMaxCount,
                        /*parent_only*/ true);
}

SegmentList GetTopChildLatentInterestSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.latent_interest_segments,
                        kTopLatentInterestSegmentsMaxCount,
                        /*parent_only*/ false);
}

SegmentList GetTopParentLatentInterestSegments(
    const UserModelInfo& user_model) {
  return GetTopSegments(user_model.latent_interest_segments,
                        kTopLatentInterestSegmentsMaxCount,
                        /*parent_only*/ true);
}

SegmentList GetTopChildPurchaseIntentSegments(const UserModelInfo& user_model) {
  return GetTopSegments(user_model.purchase_intent_segments,
                        kTopPurchaseIntentSegmentsMaxCount,
                        /*parent_only*/ false);
}

SegmentList GetTopParentPurchaseIntentSegments(
    const UserModelInfo& user_model) {
  return GetTopSegments(user_model.purchase_intent_segments,
                        kTopPurchaseIntentSegmentsMaxCount,
                        /*parent_only*/ true);
}

}  // namespace ads::targeting
