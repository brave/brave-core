/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_PROMOTION_UTILS_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_PROMOTION_UTILS_H_

#include <string>

class AutocompleteInput;
class AutocompleteResult;
struct AutocompleteMatch;

namespace brave_search_conversion {
enum class ConversionType;
}  // namespace brave_search_conversion

// Exposed for testing.
void SortBraveSearchPromotionMatch(AutocompleteResult* result);

// True when |match| is the brave search conversion promotion match for |input|.
bool IsBraveSearchPromotionMatch(const AutocompleteMatch& match);

brave_search_conversion::ConversionType GetConversionTypeFromMatch(
    const AutocompleteMatch& match);
void SetConversionTypeToMatch(brave_search_conversion::ConversionType type,
                              AutocompleteMatch* match);

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_PROMOTION_UTILS_H_
