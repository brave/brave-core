/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource_info.h"

namespace brave_ads {

OrderExtractionPatternsResourceInfo::OrderExtractionPatternsResourceInfo() =
    default;

OrderExtractionPatternsResourceInfo::OrderExtractionPatternsResourceInfo(
    OrderExtractionPatternsResourceInfo&&) noexcept = default;

OrderExtractionPatternsResourceInfo&
OrderExtractionPatternsResourceInfo::operator=(
    OrderExtractionPatternsResourceInfo&&) noexcept = default;

OrderExtractionPatternsResourceInfo::~OrderExtractionPatternsResourceInfo() =
    default;

}  // namespace brave_ads
