/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"

namespace brave_ads {

OrderValueExtractionPatternsInfo::OrderValueExtractionPatternsInfo() = default;

OrderValueExtractionPatternsInfo::OrderValueExtractionPatternsInfo(
    const OrderValueExtractionPatternsInfo&) = default;

OrderValueExtractionPatternsInfo& OrderValueExtractionPatternsInfo::operator=(
    const OrderValueExtractionPatternsInfo&) = default;

OrderValueExtractionPatternsInfo::OrderValueExtractionPatternsInfo(
    OrderValueExtractionPatternsInfo&&) noexcept = default;

OrderValueExtractionPatternsInfo& OrderValueExtractionPatternsInfo::operator=(
    OrderValueExtractionPatternsInfo&&) noexcept = default;

OrderValueExtractionPatternsInfo::~OrderValueExtractionPatternsInfo() = default;

}  // namespace brave_ads
