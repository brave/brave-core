/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"

namespace brave_ads {

OrderExtractionSelectorInfo::OrderExtractionSelectorInfo() = default;

OrderExtractionSelectorInfo::OrderExtractionSelectorInfo(
    const OrderExtractionSelectorInfo&) = default;

OrderExtractionSelectorInfo& OrderExtractionSelectorInfo::operator=(
    const OrderExtractionSelectorInfo&) = default;

OrderExtractionSelectorInfo::OrderExtractionSelectorInfo(
    OrderExtractionSelectorInfo&&) noexcept = default;

OrderExtractionSelectorInfo& OrderExtractionSelectorInfo::operator=(
    OrderExtractionSelectorInfo&&) noexcept = default;

OrderExtractionSelectorInfo::~OrderExtractionSelectorInfo() = default;

}  // namespace brave_ads
