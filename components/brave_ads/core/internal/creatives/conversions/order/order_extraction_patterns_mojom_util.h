/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MOJOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MOJOM_UTIL_H_

#include <optional>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct OrderIdExtractionPatternsInfo;
struct OrderValueExtractionPatternsInfo;

mojom::OrderExtractionPatternsInfoPtr ToMojomOrderExtractionPatterns(
    std::optional<OrderIdExtractionPatternsInfo> id,
    std::optional<OrderValueExtractionPatternsInfo> value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_MOJOM_UTIL_H_
