/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_ASK_HELPER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_ASK_HELPER_H_

#include <string_view>

struct AutocompleteMatch;

namespace brave_search {

// Detects if the input text is a question.
bool IsQuestionInput(std::string_view input);

// Transforms the destination_url of the specified match to use Ask Brave, if
// the match is a Brave Search URL and the match contents represent a question.
void MaybeTransformDestinationUrlForQuestionInput(AutocompleteMatch& match);

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_ASK_HELPER_H_
