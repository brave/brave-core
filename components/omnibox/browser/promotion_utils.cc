/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/promotion_utils.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetPromoURL;

namespace {
constexpr char kBraveSearchPromotionTypeProperty[] =
    "brave_search_promotion_type_property";
}  // namespace

void SortBraveSearchPromotionMatch(AutocompleteResult* result) {
  DCHECK(result);

  if (result->size() == 0)
    return;

  ACMatches::iterator brave_search_conversion_match = std::find_if(
      result->begin(), result->end(), [&](const AutocompleteMatch& m) {
        return IsBraveSearchPromotionMatch(m);
      });

  // Early return when |result| doesn't include promotion match.
  if (brave_search_conversion_match == result->end())
    return;

  // If first match is not from search query with default provider,
  // it means there are better matches from other providers.
  // In this case, remove the promotion match from |result|.
  // NOTE: SEARCH_WHAT_YOU_TYPED : The input is a search query (with the
  // default engine).
  if (result->begin()->type != AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED) {
    result->RemoveMatch(brave_search_conversion_match);
    return;
  }

  // Put as a second match for button type. Otherwise, put at last.
  const int target_index =
      GetConversionTypeFromMatch(*brave_search_conversion_match) ==
              ConversionType::kButton
          ? 1
          : -1;
  result->ReorderMatch(brave_search_conversion_match, target_index);
}

bool IsBraveSearchPromotionMatch(const AutocompleteMatch& match) {
  return GetConversionTypeFromMatch(match) != ConversionType::kNone;
}

ConversionType GetConversionTypeFromMatch(const AutocompleteMatch& match) {
  const std::string type_string =
      match.GetAdditionalInfo(kBraveSearchPromotionTypeProperty);
  // |match| doesn't have type info.
  if (type_string.empty())
    return ConversionType::kNone;
  int type_int;
  if (!base::StringToInt(type_string, &type_int))
    return ConversionType::kNone;
  const ConversionType type = static_cast<ConversionType>(type_int);
  DCHECK(type == ConversionType::kButton || type == ConversionType::kBanner);
  return type;
}

void SetConversionTypeToMatch(ConversionType type, AutocompleteMatch* match) {
  match->RecordAdditionalInfo(kBraveSearchPromotionTypeProperty,
                              static_cast<int>(type));
}
