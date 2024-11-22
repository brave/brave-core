/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"

#include <utility>

#include "components/omnibox/browser/autocomplete_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveSearchSuggestionParser, ParseSuggestResultsEmptyRootList) {
  base::Value::List root_list;
  AutocompleteInput input;
  bool is_keyword_result = false;
  SearchSuggestionParser::Results results;

  EXPECT_FALSE(omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                            &results));
}

TEST(BraveSearchSuggestionParser, ParseSuggestResultsEntityType) {
  base::Value::Dict suggestion;
  suggestion.Set("is_entity", base::Value(true));
  suggestion.Set("q", base::Value("helldivers 2"));
  suggestion.Set("name", base::Value("Helldivers 2"));
  suggestion.Set(
      "desc",
      base::Value("2024 video game developed by Arrowhead Game Studios"));
  suggestion.Set("category", base::Value("game"));
  suggestion.Set(
      "img",
      base::Value("https://imgs.search.brave.com/"
                  "To3SrgqTzUM9ADdXKrWxzAhplxPLgTggBSsPrF61GFo/rs:fit:60:60:1/"
                  "g:ce/aHR0cHM6Ly91cGxv/YWQud2lraW1lZGlh/Lm9yZy93aWtpcGVk/"
                  "aWEvZW4vZS9lNy9I/ZWxsZGl2ZXJzMmNv/dmVyLnBuZw"));
  suggestion.Set("logo", base::Value(false));

  base::Value::List suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::Value::List root_list;
  root_list.Append(base::Value("hel"));
  root_list.Append(base::Value(std::move(suggestion_list)));

  AutocompleteInput input;
  input.UpdateText(u"hel", /*cursor_position*/ 2, /*parts*/ {});
  bool is_keyword_result = false;
  SearchSuggestionParser::Results results;

  EXPECT_TRUE(omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                           &results));
  EXPECT_EQ(1u, results.suggest_results.size());

  const auto& result = results.suggest_results.front();

  EXPECT_EQ(u"helldivers 2", result.suggestion());
  EXPECT_EQ(u"2024 video game developed by Arrowhead Game Studios",
            result.annotation());

  EXPECT_EQ("Helldivers 2", result.entity_info().name());
  EXPECT_EQ(
      "https://imgs.search.brave.com/"
      "To3SrgqTzUM9ADdXKrWxzAhplxPLgTggBSsPrF61GFo/rs:fit:60:60:1/"
      "g:ce/aHR0cHM6Ly91cGxv/YWQud2lraW1lZGlh/Lm9yZy93aWtpcGVk/"
      "aWEvZW4vZS9lNy9I/ZWxsZGl2ZXJzMmNv/dmVyLnBuZw",
      result.entity_info().image_url());
  EXPECT_EQ("2024 video game developed by Arrowhead Game Studios",
            result.entity_info().annotation());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResultsNonEntityType) {
  base::Value::Dict suggestion;
  suggestion.Set("is_entity", base::Value(false));
  suggestion.Set("q", base::Value("1+2+3+4+...+n formula"));

  base::Value::List suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::Value::List root_list;
  root_list.Append(base::Value("1 + 2"));
  root_list.Append(base::Value(std::move(suggestion_list)));

  AutocompleteInput input;
  input.UpdateText(u"1 + 2", /*cursor_position*/ 4, /*parts*/ {});
  bool is_keyword_result = false;
  SearchSuggestionParser::Results results;

  EXPECT_TRUE(omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                           &results));
  EXPECT_EQ(1u, results.suggest_results.size());

  const auto& result = results.suggest_results.front();

  EXPECT_EQ(u"1+2+3+4+...+n formula", result.suggestion());
  EXPECT_TRUE(result.annotation().empty());

  EXPECT_FALSE(result.entity_info().has_name());
  EXPECT_FALSE(result.entity_info().has_image_url());
  EXPECT_FALSE(result.entity_info().has_annotation());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResultsFilterSVGImage) {
  base::Value::Dict suggestion;
  suggestion.Set("is_entity", base::Value(true));
  suggestion.Set("q", base::Value("helldivers 2"));
  suggestion.Set("name", base::Value("Helldivers 2"));
  suggestion.Set(
      "desc",
      base::Value("2024 video game developed by Arrowhead Game Studios"));
  suggestion.Set("category", base::Value("game"));
  suggestion.Set(
      "img",
      base::Value("https://imgs.search.brave.com/"
                  "To3SrgqTzUM9ADdXKrWxzAhplxPLgTggBSsPrF61GFo/rs:fit:60:60:1/"
                  "g:ce/aHR0cHM6Ly91cGxv/YWQud2lraW1lZGlh/Lm9yZy93aWtpcGVk/"
                  "aWEvZW4vZS9lNy9I/ZWxsZGl2ZXJzMmNv/dmVyLnBuZw.svg"));
  suggestion.Set("logo", base::Value(false));

  base::Value::List suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::Value::List root_list;
  root_list.Append(base::Value("hel"));
  root_list.Append(base::Value(std::move(suggestion_list)));

  AutocompleteInput input;
  input.UpdateText(u"hel", /*cursor_position*/ 2, /*parts*/ {});
  bool is_keyword_result = false;
  SearchSuggestionParser::Results results;

  ASSERT_TRUE(omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                           &results));
  ASSERT_EQ(1u, results.suggest_results.size());
  const auto& result = results.suggest_results.front();

  EXPECT_FALSE(result.entity_info().has_image_url());
}
