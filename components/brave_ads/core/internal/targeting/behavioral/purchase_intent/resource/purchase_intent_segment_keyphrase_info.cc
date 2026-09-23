/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_segment_keyphrase_info.h"

#include <utility>

namespace brave_ads {

PurchaseIntentSegmentKeyphraseInfo::PurchaseIntentSegmentKeyphraseInfo() =
    default;

PurchaseIntentSegmentKeyphraseInfo::PurchaseIntentSegmentKeyphraseInfo(
    SegmentList segments,
    KeywordList keywords)
    : segments(std::move(segments)), keywords(std::move(keywords)) {}

PurchaseIntentSegmentKeyphraseInfo::PurchaseIntentSegmentKeyphraseInfo(
    PurchaseIntentSegmentKeyphraseInfo&& other) noexcept = default;

PurchaseIntentSegmentKeyphraseInfo&
PurchaseIntentSegmentKeyphraseInfo::operator=(
    PurchaseIntentSegmentKeyphraseInfo&& other) noexcept = default;

PurchaseIntentSegmentKeyphraseInfo::~PurchaseIntentSegmentKeyphraseInfo() =
    default;

}  // namespace brave_ads
