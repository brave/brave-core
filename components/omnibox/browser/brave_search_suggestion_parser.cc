/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/search_suggestion_parser.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/omnibox_proto/navigational_intent.pb.h"
#include "ui/base/device_form_factor.h"
#include "ui/base/l10n/l10n_util.h"

namespace omnibox {

namespace {

// Returns the suggestion's vertical, or an empty view when absent. `type` is
// only sent when the request asks for `rich_verticals`.
std::string_view GetVerticalType(const base::DictValue& suggestion) {
  const std::string* type = suggestion.FindString("type");
  return type ? *type : std::string_view();
}

// `answer` is a JSON number for arithmetic, but a string for results like unit
// conversions.
std::optional<std::u16string> GetAnswer(const base::DictValue& suggestion) {
  if (auto* answer = suggestion.FindString("answer")) {
    return base::UTF8ToUTF16(*answer);
  }
  if (auto answer = suggestion.FindDouble("answer")) {
    return base::NumberToString16(*answer);
  }
  return std::nullopt;
}

}  // namespace

bool ParseSuggestResults(const base::ListValue& root_list,
                         const AutocompleteInput& input,
                         bool is_keyword_result,
                         SearchSuggestionParser::Results* results) {
  // `rich=true` allows for additional information for each suggestion. For
  // example, a description or image.
  //
  // Example suggestion output with `rich=true`
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

  // `rich_verticals` additionally adds a "type" field to every suggestion plus
  // vertical-specific fields.
  //
  // Example suggestion output with `rich_verticals=true`
  // [
  //    "5-2",
  //    [
  //        {
  //            "type": "calculator",
  //            "is_entity": false,
  //            "q": "5-2",
  //            "expression": "5-2",
  //            "answer": 3
  //        },
  //    ]
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
    if (suggestion_dict.FindBool("is_entity").value_or(false)) {
      // Entities are flagged by `is_entity`, not by `type`.
      suggest_type = omnibox::TYPE_ENTITY;
      match_type = AutocompleteMatchType::SEARCH_SUGGEST_ENTITY;
    } else if (GetVerticalType(suggestion_dict) == "calculator") {
      // Verticals we don't handle stay plain query suggestions.
      suggest_type = omnibox::TYPE_CALCULATOR;
      match_type = AutocompleteMatchType::CALCULATOR;
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

    std::u16string suggestion_text;
    std::u16string match_contents;
    if (suggest_type == omnibox::TYPE_CALCULATOR) {
      auto answer = GetAnswer(suggestion_dict);
      if (!answer || answer->empty()) {
        continue;
      }
      // An annotation becomes the match description, which restores the
      // separator the desktop match cell suppresses for CALCULATOR -- the row
      // would read "<answer> - <description>".
      annotation.clear();
      // The suggestion is the answer, so accepting the match searches the text
      // the user typed. See BaseSearchProvider::CreateSearchSuggestion.
      suggestion_text = std::move(*answer);
      match_contents = suggestion_text;
      if (ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_DESKTOP) {
        // Desktop shows "<expression> = <answer>" on one line, as upstream
        // does.
        const auto* expression = suggestion_dict.FindString("expression");
        match_contents = l10n_util::GetStringFUTF16(
            IDS_OMNIBOX_ONE_LINE_CALCULATOR_SUGGESTION_TEMPLATE,
            base::UTF8ToUTF16(expression ? *expression : *search_query),
            suggestion_text);
      }
    } else {
      suggestion_text = base::UTF8ToUTF16(*search_query);
      match_contents = suggestion_text;
    }

    auto result = SearchSuggestionParser::SuggestResult(
        suggestion_text, match_type, suggest_type,
        /*subtypes*/ {}, match_contents,
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
