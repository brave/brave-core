/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_segment_keyword_info.h"

namespace ads::targeting {

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo() = default;

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo(
    const SegmentList& segments,
    const std::string& keywords)
    : segments(segments), keywords(keywords) {}

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo(
    const PurchaseIntentSegmentKeywordInfo& info) = default;

PurchaseIntentSegmentKeywordInfo& PurchaseIntentSegmentKeywordInfo::operator=(
    const PurchaseIntentSegmentKeywordInfo& info) = default;

PurchaseIntentSegmentKeywordInfo::~PurchaseIntentSegmentKeywordInfo() = default;

}  // namespace ads::targeting
