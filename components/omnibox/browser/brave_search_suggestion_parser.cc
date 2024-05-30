/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/search_suggestion_parser.h"
#include "third_party/omnibox_proto/navigational_intent.pb.h"

namespace omnibox {

bool ParseSuggestResults(const base::Value::List& root_list,
                         const AutocompleteInput& input,
                         bool is_keyword_result,
                         SearchSuggestionParser::Results* results) {
  // Example output of rich suggestion
  // 1) Type "hel"
  // [
  //     "hel",
  //     [
  //         {
  //             "is_entity": true,
  //             "q": "helldivers 2",
  //             "name": "Helldivers 2",
  //             "desc": "2024 video game developed by Arrowhead Game
  //             Studios", "category": "game", "img":
  //             "https://imgs.search.brave.com/To3SrgqTzUM9ADdXKrWxzAhplxPLgTggBSsPrF61GFo/rs:fit:60:60:1/g:ce/aHR0cHM6Ly91cGxv/YWQud2lraW1lZGlh/Lm9yZy93aWtpcGVk/aWEvZW4vZS9lNy9I/ZWxsZGl2ZXJzMmNv/dmVyLnBuZw",
  //             "logo": false
  //         },
  //     ]
  // ]
  //
  // 2) Type 1 + 2
  // [
  //     "1 + 2",
  //     [
  //         {
  //             "is_entity": false,
  //             "q": "1+2+3+4+...+n formula"
  //         },
  //     ]
  // ]
  const std::u16string input_text = input.IsZeroSuggest() ? u"" : input.text();

  // 1st element: query.
  if (root_list.empty() || !root_list[0].is_string()) {
    return false;
  }
  std::u16string query = base::UTF8ToUTF16(root_list[0].GetString());
  if (query != input_text) {
    return false;
  }
  // 2nd element: suggestions list.
  if (root_list.size() < 2u || !root_list[1].is_list()) {
    return false;
  }

  results->verbatim_relevance = -1;
  results->field_trial_triggered = false;
  results->suggest_results.clear();
  results->navigation_results.clear();

  for (const auto& suggestion : root_list[1].GetList()) {
    if (!suggestion.is_dict()) {
      continue;
    }

    const auto& suggestion_dict = suggestion.GetDict();
    auto* search_query = suggestion_dict.FindString("q");
    if (!search_query) {
      continue;
    }

    AutocompleteMatchType::Type match_type =
        AutocompleteMatchType::SEARCH_SUGGEST;
    omnibox::SuggestType suggest_type = omnibox::TYPE_QUERY;
    omnibox::EntityInfo entity_info;
    if (auto is_entity = suggestion_dict.FindBool("is_entity");
        is_entity.value_or(false)) {
      suggest_type = omnibox::TYPE_ENTITY;
      match_type = AutocompleteMatchType::SEARCH_SUGGEST_ENTITY;
    }

    if (auto* name = suggestion_dict.FindString("name")) {
      entity_info.set_name(*name);
    }

    if (auto* image_url = suggestion_dict.FindString("img");
        image_url && !image_url->empty() && !image_url->ends_with(".svg")) {
      // As Native UI can't render svg, we should filter them out. Notably,
      // OmniboxMatchCell is getting valid image even when it's svg and it
      // decides weather to render it or not based on the URL, this would be
      // an easy way to show magnifying glass icon for svg images.
      entity_info.set_image_url(*image_url);
    }

    auto* description = suggestion_dict.FindString("desc");
    std::u16string annotation;
    if (description && !description->empty()) {
      annotation = base::UTF8ToUTF16(*description);
      entity_info.set_annotation(*description);
    }

    const auto search_query_in_utf16 = base::UTF8ToUTF16(*search_query);
    auto result = SearchSuggestionParser::SuggestResult(
        search_query_in_utf16, match_type, suggest_type,
        /*subtypes*/ {},
        /*match_contents*/ search_query_in_utf16,
        /*match_contents_prefix*/ {},
        /*annotation*/ annotation, std::move(entity_info),
        /*deletion_url*/ {}, is_keyword_result, omnibox::NAV_INTENT_NONE,
        /*relevance*/ -1,
        /*relevance_from_server*/ false, false, false,
        base::CollapseWhitespace(input_text, false));

    results->suggest_results.push_back(std::move(result));
  }
  return true;
}

}  // namespace omnibox
