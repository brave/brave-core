/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_default.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_default_constants.h"

namespace brave_ads {

const OrderExtractionPatternsInfo& GetDefaultOrderExtractionPatterns() {
  static const base::NoDestructor<OrderExtractionPatternsInfo> kPatterns([] {
    OrderExtractionPatternsInfo patterns;

    patterns.value.currency_regexes["EUR"] = kDefaultEurOrderValueCurrencyRegex;
    patterns.value.currency_regexes["GBP"] = kDefaultGbpOrderValueCurrencyRegex;
    patterns.value.currency_regexes["USD"] = kDefaultUsdOrderValueCurrencyRegex;
    patterns.value.keyword_regexes["de"] = kDefaultDeOrderValueKeywordRegex;
    patterns.value.keyword_regexes["en"] = kDefaultEnOrderValueKeywordRegex;
    patterns.value.keyword_regexes["fr"] = kDefaultFrOrderValueKeywordRegex;
    patterns.value.default_number_of_ancestor_levels_to_search =
        kDefaultNumberOfAncestorLevelsToSearch;

    patterns.id.regexes["de"] = kDefaultDeOrderIdRegex;
    patterns.id.regexes["en"] = kDefaultEnOrderIdRegex;
    patterns.id.regexes["fr"] = kDefaultFrOrderIdRegex;
    patterns.id.default_max_combined_text_length_to_search =
        kDefaultMaxCombinedTextLengthToSearch;
    patterns.id.default_number_of_descendant_levels_to_search =
        kDefaultNumberOfDescendantLevelsToSearch;
    patterns.id.default_validation_pattern = kDefaultIdValidationPattern;

    return patterns;
  }());
  return *kPatterns;
}

}  // namespace brave_ads
