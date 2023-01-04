/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_segment_keyword_info.h"

#include <utility>

namespace ads::targeting {

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo() = default;

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo(
    SegmentList segments,
    std::string keywords)
    : segments(std::move(segments)), keywords(std::move(keywords)) {}

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo(
    const PurchaseIntentSegmentKeywordInfo& other) = default;

PurchaseIntentSegmentKeywordInfo& PurchaseIntentSegmentKeywordInfo::operator=(
    const PurchaseIntentSegmentKeywordInfo& other) = default;

PurchaseIntentSegmentKeywordInfo::PurchaseIntentSegmentKeywordInfo(
    PurchaseIntentSegmentKeywordInfo&& other) noexcept = default;

PurchaseIntentSegmentKeywordInfo& PurchaseIntentSegmentKeywordInfo::operator=(
    PurchaseIntentSegmentKeywordInfo&& other) noexcept = default;

PurchaseIntentSegmentKeywordInfo::~PurchaseIntentSegmentKeywordInfo() = default;

}  // namespace ads::targeting
