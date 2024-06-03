/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_SUGGESTION_PARSER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_SUGGESTION_PARSER_H_

#include "components/omnibox/browser/search_suggestion_parser.h"

class AutocompleteInput;

namespace omnibox {

bool ParseSuggestResults(const base::Value::List& root_list,
                         const AutocompleteInput& input,
                         bool is_keyword_result,
                         SearchSuggestionParser::Results* results);

}  // namespace omnibox

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_SUGGESTION_PARSER_H_
