/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/search_suggestion_parser.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "third_party/omnibox_proto/types.pb.h"

#define ParseSuggestResults ParseSuggestResults_Chromium

#include "src/components/omnibox/browser/search_suggestion_parser.cc"

#undef ParseSuggestResults

// static
bool SearchSuggestionParser::ParseSuggestResults(
    const base::Value::List& root_list,
    const AutocompleteInput& input,
    const AutocompleteSchemeClassifier& scheme_classifier,
    int default_result_relevance,
    bool is_keyword_result,
    Results* results,
    bool is_brave_rich_suggestion) {
  if (is_brave_rich_suggestion) {
    return omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                        results);
  }
  return SearchSuggestionParser::ParseSuggestResults_Chromium(
      root_list, input, scheme_classifier, default_result_relevance,
      is_keyword_result, results);
}
