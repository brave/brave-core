/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/promotion_utils.h"

#include <algorithm>

#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"

using brave_search_conversion::ConversionType;

void SortBraveSearchPromotionMatch(
    AutocompleteResult* result,
    const AutocompleteInput& input,
    brave_search_conversion::ConversionType type) {
  if (result->size() == 0)
    return;

  if (brave_search_conversion::ConversionType::kNone == type)
    return;

  ACMatches::iterator brave_search_conversion_match = std::find_if(
      result->begin(), result->end(), [&](const AutocompleteMatch& m) {
        return IsBraveSearchPromotionMatch(m, input.text());
      });

  // Early return when |result| doesn't include promotion match.
  if (brave_search_conversion_match == result->end())
    return;

  // If first match is not from search query with default provider,
  // it means there is more proper match from other providers.
  // In this case, remove promotion match from |result|.
  // NOTE: SEARCH_WHAT_YOU_TYPED : The input as a search query (with the
  // default engine).
  if (result->begin()->type != AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED) {
    result->RemoveMatch(brave_search_conversion_match);
    return;
  }

  const size_t from_index = brave_search_conversion_match - result->begin();
  // Put as a second match for button type. Otherwise, put at last.
  const size_t to_index = type == ConversionType::kButton ? 1 : result->size();
  result->MoveMatch(from_index, to_index);
}

bool IsBraveSearchPromotionMatch(const AutocompleteMatch& match,
                                 const std::u16string& input) {
  return match.type == AutocompleteMatchType::NAVSUGGEST &&
         match.destination_url == brave_search_conversion::GetPromoURL(input);
}
