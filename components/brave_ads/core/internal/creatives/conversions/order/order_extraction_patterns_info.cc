/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"

namespace brave_ads {

OrderExtractionPatternsInfo::OrderExtractionPatternsInfo() = default;

OrderExtractionPatternsInfo::OrderExtractionPatternsInfo(
    const OrderExtractionPatternsInfo&) = default;

OrderExtractionPatternsInfo& OrderExtractionPatternsInfo::operator=(
    const OrderExtractionPatternsInfo&) = default;

OrderExtractionPatternsInfo::OrderExtractionPatternsInfo(
    OrderExtractionPatternsInfo&&) noexcept = default;

OrderExtractionPatternsInfo& OrderExtractionPatternsInfo::operator=(
    OrderExtractionPatternsInfo&&) noexcept = default;

OrderExtractionPatternsInfo::~OrderExtractionPatternsInfo() = default;

}  // namespace brave_ads
