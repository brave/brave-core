/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_UTIL_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct ComponentOrderIdExtractionPatternsInfo;
struct ComponentOrderValueExtractionPatternsInfo;

// Converts component-level patterns to site-specific Mojom patterns, trying
// `site_candidates` most specific first and `detected_language` regexes
// first; with no value, tries every language in schema order.
mojom::OrderIdExtractionPatternsInfoPtr ToOrderIdExtractionPatterns(
    const ComponentOrderIdExtractionPatternsInfo&
        component_order_id_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::optional<std::string_view> detected_language);

// Converts component-level patterns to site-specific Mojom patterns, trying
// `site_candidates` most specific first and `detected_language` regexes
// first; with no value, tries every language in schema order.
mojom::OrderValueExtractionPatternsInfoPtr ToOrderValueExtractionPatterns(
    const ComponentOrderValueExtractionPatternsInfo&
        component_order_value_extraction_patterns,
    base::span<const std::string> site_candidates,
    std::optional<std::string_view> detected_language);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_UTIL_H_
