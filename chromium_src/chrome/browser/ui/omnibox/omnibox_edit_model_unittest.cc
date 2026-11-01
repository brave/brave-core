/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/omnibox/omnibox_edit_model_unittest.cc>

// Regression test for a stale `popup_selection_.line` crashing when
// computing the accessibility label.
TEST_F(OmniboxEditModelPopupTest,
       GetPopupAccessibilityLabelForCurrentSelection_StaleSelection) {
  // 1. Start with 2 matches, select the second (index 1).
  ACMatches matches;
  AutocompleteMatch match1(nullptr, 1000, false,
                           omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
  matches.push_back(match1);
  AutocompleteMatch match2(nullptr, 500, false,
                           omnibox::AutocompleteMatchType::kSearchSuggest);
  matches.push_back(match2);

  auto* result = &AutocompleteControllerPublishedResult();
  result->AppendMatches(matches);

  model()->SetPopupSelection(
      OmniboxPopupSelection(1, OmniboxPopupSelection::LineState::kNormal));
  ASSERT_EQ(1u, model()->GetPopupSelection().line);

  // 2. Update the result set to have only 1 match, same as above.
  ACMatches new_matches;
  new_matches.push_back(match1);
  result->Reset();
  result->AppendMatches(new_matches);

  // 3. Compute the label for the now out-of-bounds selection.
  // If the selection isn't checked first, this will try to access
  // index 1 on a size 1 result, causing a crash.
  int label_prefix_length = 0;
  std::u16string label = model()->GetPopupAccessibilityLabelForCurrentSelection(
      u"user input", true, &label_prefix_length);

  // 4. Verify an empty label was returned instead of crashing.
  EXPECT_EQ(std::u16string(), label);
}

// Regression test for the same stale `popup_selection_.line` as
// above, reached from a different caller: `CurrentMatch()` ->
// `GetInfoForCurrentText()`.
TEST_F(OmniboxEditModelPopupTest, CurrentMatchHandlesStaleSelection) {
  // 1. Start with 2 matches, select the second (index 1).
  ACMatches matches;
  AutocompleteMatch match1(nullptr, 1000, false,
                           omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
  matches.push_back(match1);
  AutocompleteMatch match2(nullptr, 500, false,
                           omnibox::AutocompleteMatchType::kSearchSuggest);
  matches.push_back(match2);

  auto* result = &AutocompleteControllerPublishedResult();
  result->AppendMatches(matches);

  model()->SetPopupSelection(
      OmniboxPopupSelection(1, OmniboxPopupSelection::LineState::kNormal));
  ASSERT_EQ(1u, model()->GetPopupSelection().line);

  // 2. Update the result set to have only 1 match.
  ACMatches new_matches;
  new_matches.push_back(match1);
  result->Reset();
  result->AppendMatches(new_matches);

  // 3. Call CurrentMatch(). If the selection isn't checked first,
  // this will try to access index 1 on a size 1 result, causing a
  // crash.
  model()->CurrentMatch();
}

TEST_F(OmniboxEditModelPopupTest, CurrentMatchReturnsSelectedMatch) {
  ACMatches matches;
  AutocompleteMatch match1(nullptr, 1000, false,
                           omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
  matches.push_back(match1);
  AutocompleteMatch match2(nullptr, 500, false,
                           omnibox::AutocompleteMatchType::kSearchSuggest);
  matches.push_back(match2);

  auto* result = &AutocompleteControllerPublishedResult();
  result->AppendMatches(matches);

  model()->SetPopupSelection(
      OmniboxPopupSelection(1, OmniboxPopupSelection::LineState::kNormal));

  AutocompleteMatch match = model()->CurrentMatch();
  EXPECT_EQ(omnibox::AutocompleteMatchType::kSearchSuggest, match.type);
}

TEST_F(OmniboxEditModelPopupTest,
       GetPopupAccessibilityLabelForCurrentSelection_EmptyResult) {
  ACMatches matches;
  AutocompleteMatch match(nullptr, 1000, false,
                          omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
  matches.push_back(match);

  auto* result = &AutocompleteControllerPublishedResult();
  result->AppendMatches(matches);

  model()->SetPopupSelection(
      OmniboxPopupSelection(0, OmniboxPopupSelection::LineState::kNormal));
  ASSERT_EQ(0u, model()->GetPopupSelection().line);

  result->Reset();
  ASSERT_EQ(0u, result->size());

  int label_prefix_length = 0;
  std::u16string label = model()->GetPopupAccessibilityLabelForCurrentSelection(
      u"user input", true, &label_prefix_length);
  EXPECT_EQ(std::u16string(), label);
}

TEST_F(OmniboxEditModelPopupTest, CurrentMatchHandlesEmptyResult) {
  ACMatches matches;
  AutocompleteMatch match(nullptr, 1000, false,
                          omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
  matches.push_back(match);

  auto* result = &AutocompleteControllerPublishedResult();
  result->AppendMatches(matches);

  model()->SetPopupSelection(
      OmniboxPopupSelection(0, OmniboxPopupSelection::LineState::kNormal));
  ASSERT_EQ(0u, model()->GetPopupSelection().line);

  result->Reset();
  ASSERT_EQ(0u, result->size());

  model()->CurrentMatch();
}
