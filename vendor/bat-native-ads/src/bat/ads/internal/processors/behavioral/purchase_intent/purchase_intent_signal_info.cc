/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/purchase_intent/purchase_intent_signal_info.h"

namespace ads::targeting {

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo() = default;

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo(
    const PurchaseIntentSignalInfo& other) = default;

PurchaseIntentSignalInfo& PurchaseIntentSignalInfo::operator=(
    const PurchaseIntentSignalInfo& other) = default;

PurchaseIntentSignalInfo::PurchaseIntentSignalInfo(
    PurchaseIntentSignalInfo&& other) noexcept = default;

PurchaseIntentSignalInfo& PurchaseIntentSignalInfo::operator=(
    PurchaseIntentSignalInfo&& other) noexcept = default;

PurchaseIntentSignalInfo::~PurchaseIntentSignalInfo() = default;

}  // namespace ads::targeting
