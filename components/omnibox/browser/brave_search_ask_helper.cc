/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_ask_helper.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/common/omnibox_features.h"
#include "url/gurl.h"

namespace brave_search {

bool IsQuestionInput(std::string_view input) {
  if (input.empty()) {
    return false;
  }

  // Trim whitespace.
  input = base::TrimString(input, base::kWhitespaceASCII, base::TRIM_ALL);

  if (input.empty()) {
    return false;
  }

  // Check if it ends with a question mark.
  if (input.back() == '?') {
    return true;
  }

  // Common question words/phrases at the start of a question.
  static constexpr std::string_view kQuestionStarters[] = {
      "who ",      "what ",     "when ",    "where ",    "why ",
      "how ",      "which ",    "whose ",   "whom ",     "can ",
      "could ",    "would ",    "should ",  "will ",     "is ",
      "are ",      "was ",      "were ",    "do ",       "does ",
      "did ",      "have ",     "has ",     "had ",      "may ",
      "might ",    "shall ",    "must ",    "ought ",    "need ",
      "dare ",     "isn't ",    "aren't ",  "wasn't ",   "weren't ",
      "doesn't ",  "didn't ",   "haven't ", "hasn't ",   "hadn't ",
      "won't ",    "wouldn't ", "can't ",   "couldn't ", "shouldn't ",
      "mightn't ", "mustn't "};

  // Convert input to lowercase for comparison.
  std::string input_lower = base::ToLowerASCII(input);

  // Check if input starts with any question word/phrase.
  for (const auto& starter : kQuestionStarters) {
    if (base::StartsWith(input_lower, starter, base::CompareCase::SENSITIVE)) {
      return true;
    }
  }

  return false;
}

void MaybeTransformDestinationUrlForQuestionInput(AutocompleteMatch& match) {
  if (!base::FeatureList::IsEnabled(omnibox::kRouteQuestionsToAskBrave)) {
    return;
  }

  if (!match.destination_url.is_valid() ||
      !match.destination_url.SchemeIsHTTPOrHTTPS() ||
      match.destination_url.host_piece() != "search.brave.com" ||
      match.destination_url.path_piece() != "/search") {
    return;
  }

  // Check if the query is a question.
  if (!IsQuestionInput(base::UTF16ToUTF8(match.contents))) {
    return;
  }

  // Transform the URL by replacing "/search" with "/ask".
  GURL::Replacements replacements;
  replacements.SetPathStr("/ask");
  match.destination_url = match.destination_url.ReplaceComponents(replacements);
}

}  // namespace brave_search
