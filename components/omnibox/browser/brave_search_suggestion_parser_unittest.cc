/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_search_suggestion_parser.h"

#include <string_view>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/test/values_test_util.h"
#include "components/omnibox/browser/autocomplete_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/device_form_factor.h"

namespace {

// Parses `suggestions_json` -- the second element of a suggest response -- as
// though the user had typed `input_text`.
bool ParseSuggestions(std::string_view input_text,
                      std::string_view suggestions_json,
                      SearchSuggestionParser::Results* results) {
  base::ListValue root_list;
  root_list.Append(input_text);
  root_list.Append(base::test::ParseJsonList(suggestions_json));

  const std::u16string input_text16 = base::UTF8ToUTF16(input_text);
  AutocompleteInput input;
  input.UpdateText(input_text16, input_text16.size(), /*parts*/ {});
  return omnibox::ParseSuggestResults(root_list, input,
                                      /*is_keyword_result*/ false, results);
}

}  // namespace

TEST(BraveSearchSuggestionParser, ParseSuggestResults_EmptyRootList) {
  base::ListValue root_list;
  AutocompleteInput input;
  bool is_keyword_result = false;
  SearchSuggestionParser::Results results;

  EXPECT_FALSE(omnibox::ParseSuggestResults(root_list, input, is_keyword_result,
                                            &results));
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_EntityType) {
  base::DictValue suggestion;
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

  base::ListValue suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::ListValue root_list;
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

TEST(BraveSearchSuggestionParser, ParseSuggestResults_NonEntityType) {
  base::DictValue suggestion;
  suggestion.Set("is_entity", base::Value(false));
  suggestion.Set("q", base::Value("1+2+3+4+...+n formula"));

  base::ListValue suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::ListValue root_list;
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

TEST(BraveSearchSuggestionParser, ParseSuggestResults_FilterSVGImage) {
  base::DictValue suggestion;
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

  base::ListValue suggestion_list;
  suggestion_list.Append(std::move(suggestion));

  base::ListValue root_list;
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

TEST(BraveSearchSuggestionParser, ParseSuggestResults_CalculatorVertical) {
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions("5-2", R"([
      {"type": "calculator", "q": "5-2", "expression": "5-2", "answer": 3},
      {"type": "query", "q": "52 states of america list"}
  ])",
                               &results));
  ASSERT_EQ(2u, results.suggest_results.size());

  // The suggestion is the answer, so accepting the match searches the typed
  // text. Desktop additionally shows "<expression> = <answer>" as the contents.
  const auto& calculator = results.suggest_results[0];
  EXPECT_EQ(AutocompleteMatchType::CALCULATOR, calculator.type());
  EXPECT_EQ(omnibox::TYPE_CALCULATOR, calculator.suggest_type());
  EXPECT_EQ(u"3", calculator.suggestion());
  if (ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_DESKTOP) {
    EXPECT_EQ(u"5-2 = 3", calculator.match_contents());
  } else {
    EXPECT_EQ(u"3", calculator.match_contents());
  }
  // A description would resurrect the separator that the desktop match cell
  // suppresses for CALCULATOR.
  EXPECT_TRUE(calculator.annotation().empty());

  // An explicit "query" type is still a plain search suggestion.
  const auto& query = results.suggest_results[1];
  EXPECT_EQ(AutocompleteMatchType::SEARCH_SUGGEST, query.type());
  EXPECT_EQ(omnibox::TYPE_QUERY, query.suggest_type());
  EXPECT_EQ(u"52 states of america list", query.suggestion());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_UnknownVertical) {
  // An unrecognized vertical degrades to a plain query suggestion rather than
  // being dropped.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions(
      "weather", R"([{"type": "weather", "q": "weather in toronto"}])",
      &results));
  ASSERT_EQ(1u, results.suggest_results.size());
  EXPECT_EQ(AutocompleteMatchType::SEARCH_SUGGEST,
            results.suggest_results[0].type());
  EXPECT_EQ(u"weather in toronto", results.suggest_results[0].suggestion());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_CalculatorStringAnswer) {
  // Unit conversions send `answer` as a string rather than a number.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions("5000 m in km", R"([
      {"type": "calculator", "q": "5000 m in km",
       "expression": "5000 m in km", "answer": "5 km"}
  ])",
                               &results));
  ASSERT_EQ(1u, results.suggest_results.size());

  const auto& calculator = results.suggest_results[0];
  EXPECT_EQ(AutocompleteMatchType::CALCULATOR, calculator.type());
  EXPECT_EQ(u"5 km", calculator.suggestion());
  if (ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_DESKTOP) {
    EXPECT_EQ(u"5000 m in km = 5 km", calculator.match_contents());
  } else {
    EXPECT_EQ(u"5 km", calculator.match_contents());
  }
}

TEST(BraveSearchSuggestionParser,
     ParseSuggestResults_CalculatorWithoutExpression) {
  // Without `expression` the query stands in for it.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions(
      "5-2", R"([{"type": "calculator", "q": "5-2", "answer": 3}])", &results));
  ASSERT_EQ(1u, results.suggest_results.size());

  const auto& calculator = results.suggest_results[0];
  EXPECT_EQ(u"3", calculator.suggestion());
  if (ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_DESKTOP) {
    EXPECT_EQ(u"5-2 = 3", calculator.match_contents());
  } else {
    EXPECT_EQ(u"3", calculator.match_contents());
  }
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_CalculatorIgnoresDesc) {
  // A description would restore the separator the desktop match cell
  // suppresses for CALCULATOR, so it is dropped even if the server sends one.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions("5-2", R"([
      {"type": "calculator", "q": "5-2", "expression": "5-2", "answer": 3,
       "desc": "arithmetic"}
  ])",
                               &results));
  ASSERT_EQ(1u, results.suggest_results.size());
  EXPECT_TRUE(results.suggest_results[0].annotation().empty());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_CalculatorWithoutAnswer) {
  // Nothing to display, so the suggestion is skipped.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions(
      "5-2", R"([{"type": "calculator", "q": "5-2", "expression": "5-2"}])",
      &results));
  EXPECT_TRUE(results.suggest_results.empty());
}

TEST(BraveSearchSuggestionParser, ParseSuggestResults_EntityWinsOverType) {
  // `is_entity` predates `type` and stays authoritative.
  SearchSuggestionParser::Results results;
  ASSERT_TRUE(ParseSuggestions("hel", R"([
      {"type": "query", "is_entity": true, "q": "helldivers 2",
       "name": "Helldivers 2"}
  ])",
                               &results));
  ASSERT_EQ(1u, results.suggest_results.size());
  EXPECT_EQ(AutocompleteMatchType::SEARCH_SUGGEST_ENTITY,
            results.suggest_results[0].type());
  EXPECT_EQ(omnibox::TYPE_ENTITY, results.suggest_results[0].suggest_type());
}
