/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_FLATTENER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_FLATTENER_H_

#include <string>
#include <string_view>

#include "base/containers/span.h"

namespace brave_ads {

struct ComponentOrderIdExtractionPatternsInfo;
struct OrderIdExtractionPatternsInfo;
struct ComponentOrderValueExtractionPatternsInfo;
struct OrderValueExtractionPatternsInfo;

// `domain_candidates` must be pre-normalized to eTLD+1, most-specific
// first, since core/internal cannot do registry-controlled-domain lookups.
OrderIdExtractionPatternsInfo FlattenOrderIdExtractionPatterns(
    const ComponentOrderIdExtractionPatternsInfo& order_id_extraction_patterns,
    base::span<const std::string> domain_candidates,
    std::string_view detected_language);

OrderValueExtractionPatternsInfo FlattenOrderValueExtractionPatterns(
    const ComponentOrderValueExtractionPatternsInfo&
        order_value_extraction_patterns,
    base::span<const std::string> domain_candidates,
    std::string_view detected_language);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_FLATTENER_H_
