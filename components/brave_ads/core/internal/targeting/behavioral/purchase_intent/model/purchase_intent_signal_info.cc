/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_signal_info.h"

#include <utility>

namespace brave_ads {

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo() = default;

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo(base::Time at,
                                                   SegmentList segments,
                                                   int weight)
    : at(at), segments(std::move(segments)), weight(weight) {}

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo(
    const PurchaseIntentSignalInfo& other) = default;

PurchaseIntentSignalInfo& PurchaseIntentSignalInfo::operator=(
    const PurchaseIntentSignalInfo& other) = default;

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo(
    PurchaseIntentSignalInfo&& other) noexcept = default;

PurchaseIntentSignalInfo& PurchaseIntentSignalInfo::operator=(
    PurchaseIntentSignalInfo&& other) noexcept = default;

PurchaseIntentSignalInfo::~PurchaseIntentSignalInfo() = default;

}  // namespace brave_ads
