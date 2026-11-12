/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"

namespace brave_ads {

OrderIdExtractionPatternsInfo::OrderIdExtractionPatternsInfo() = default;

OrderIdExtractionPatternsInfo::OrderIdExtractionPatternsInfo(
    const OrderIdExtractionPatternsInfo&) = default;

OrderIdExtractionPatternsInfo& OrderIdExtractionPatternsInfo::operator=(
    const OrderIdExtractionPatternsInfo&) = default;

OrderIdExtractionPatternsInfo::OrderIdExtractionPatternsInfo(
    OrderIdExtractionPatternsInfo&&) noexcept = default;

OrderIdExtractionPatternsInfo& OrderIdExtractionPatternsInfo::operator=(
    OrderIdExtractionPatternsInfo&&) noexcept = default;

OrderIdExtractionPatternsInfo::~OrderIdExtractionPatternsInfo() = default;

}  // namespace brave_ads
